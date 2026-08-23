#!/usr/bin/env bash
set -euo pipefail

# Packages the canonical Xcode Release artefacts. The AUv3 extension is not
# copied to a system plug-in directory: macOS registers it from the Standalone
# app bundle where JUCE embeds it as an app extension.

if [[ "$(uname -s)" != "Darwin" ]]; then
    printf '%s\n' "This packaging script must run on macOS." >&2
    exit 1
fi

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${1:-"$repo_root/build-mac"}"
configuration="${CONFIGURATION:-Release}"

if [[ "$build_dir" != /* ]]; then
    build_dir="$repo_root/$build_dir"
fi

version="$({ sed -nE 's/^[[:space:]]*project[[:space:]]*\([[:space:]]*UTT[[:space:]]+VERSION[[:space:]]+([0-9]+\.[0-9]+\.[0-9]+).*/\1/p' "$repo_root/CMakeLists.txt"; } | head -n 1)"
if [[ -z "$version" ]]; then
    printf '%s\n' "Could not read the UTT product version from CMakeLists.txt." >&2
    exit 1
fi

release_root="$build_dir/UTT_artefacts/$configuration"
vst3_source="$release_root/VST3/UTT.vst3"
au_source="$release_root/AU/UTT.component"
standalone_source="$release_root/Standalone/UTT.app"
embedded_auv3="$standalone_source/Contents/PlugIns/UTT.appex"

for artefact in "$vst3_source" "$au_source" "$standalone_source" "$embedded_auv3"; do
    if [[ ! -d "$artefact" ]]; then
        printf 'Missing canonical macOS artefact: %s\n' "$artefact" >&2
        exit 1
    fi
done

find_bundle_binary() {
    local bundle="$1"
    local binary=""

    for candidate in "$bundle"/Contents/MacOS/*; do
        if [[ -f "$candidate" ]]; then
            binary="$candidate"
            break
        fi
    done

    if [[ -z "$binary" ]]; then
        printf 'No executable found in bundle: %s\n' "$bundle" >&2
        exit 1
    fi

    printf '%s\n' "$binary"
}

require_universal_bundle() {
    local bundle="$1"
    local label="$2"
    local binary
    local architectures

    binary="$(find_bundle_binary "$bundle")"
    architectures="$(lipo -archs "$binary")"
    for architecture in x86_64 arm64; do
        if [[ " $architectures " != *" $architecture "* ]]; then
            printf '%s is missing %s support: %s\n' "$label" "$architecture" "$architectures" >&2
            exit 1
        fi
    done
}

require_universal_bundle "$vst3_source" "VST3"
require_universal_bundle "$au_source" "AU"
require_universal_bundle "$standalone_source" "Standalone"
require_universal_bundle "$embedded_auv3" "embedded AUv3"

adhoc_sign_bundle() {
    local bundle="$1"

    codesign --force --deep --sign - "$bundle"
    codesign --verify --deep --strict "$bundle"
}

# A GitHub runner has no Developer ID certificate. Ad-hoc signatures make the
# nested AUv3 extension a valid app-bundle member, while the package itself is
# still deliberately marked unsigned/not notarized for distribution purposes.
adhoc_sign_bundle "$vst3_source"
adhoc_sign_bundle "$au_source"
adhoc_sign_bundle "$standalone_source"

dist_root="$repo_root/dist"
package_name="UTT-$version-macos-universal-unsigned.pkg"
package_path="$dist_root/$package_name"
hash_path="$package_path.sha256"
stage_root="$(mktemp -d "${TMPDIR:-/tmp}/utt-package.XXXXXX")"

cleanup() {
    rm -rf "$stage_root"
}
trap cleanup EXIT

mkdir -p "$dist_root"
rm -f "$package_path" "$hash_path"
mkdir -p "$stage_root/Library/Audio/Plug-Ins/VST3"
mkdir -p "$stage_root/Library/Audio/Plug-Ins/Components"
mkdir -p "$stage_root/Applications"

ditto "$vst3_source" "$stage_root/Library/Audio/Plug-Ins/VST3/UTT.vst3"
ditto "$au_source" "$stage_root/Library/Audio/Plug-Ins/Components/UTT.component"
ditto "$standalone_source" "$stage_root/Applications/UTT.app"

pkgbuild \
    --root "$stage_root" \
    --identifier "com.unpurebloom.utt" \
    --version "$version" \
    --install-location / \
    "$package_path"

printf '%s  %s\n' "$(shasum -a 256 "$package_path" | awk '{ print $1 }')" "$package_name" > "$hash_path"

printf 'Built unsigned macOS package: %s\n' "$package_path"
printf '%s\n' "Installs ad-hoc-signed VST3, AU, and UTT.app with its embedded AUv3 extension."
