using System;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.Reflection;
using System.Windows.Forms;

class UTTInstaller
{
    private const string ProductName = "UTT - Utility Toolbox";
    private const string ProductVersion = "0.3.1";
    private const string ZipResource = "UTTInstaller.Payload.utt_data.zip";
    private const string WebViewResource = "UTTInstaller.Payload.MicrosoftEdgeWebView2RuntimeInstallerX64.exe";
    private const string VcRedistResource = "UTTInstaller.Payload.VC_redist.x64.exe";
    private const string LicenseResource = "UTTInstaller.Payload.LICENSE.txt";

    [STAThread]
    static int Main(string[] args)
    {
        var quiet = Array.Exists(args, arg => string.Equals(arg, "--quiet", StringComparison.OrdinalIgnoreCase));
        Application.EnableVisualStyles();
        Application.SetCompatibleTextRenderingDefault(false);

        var vst3Root = Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.CommonProgramFiles),
            "VST3");
        var targetDir = Path.Combine(vst3Root, "UTT.vst3");

        if (!quiet)
        {
            var confirm = MessageBox.Show(
                ProductName + "\nVersion " + ProductVersion + "\n\n"
                + "This installer adds the required WebView2 and VC++ runtimes, then installs:\n"
                + targetDir + "\n\nContinue?",
                "UTT Installer",
                MessageBoxButtons.YesNo,
                MessageBoxIcon.Question);
            if (confirm != DialogResult.Yes)
                return 0;
        }

        var tempRoot = Path.Combine(Path.GetTempPath(), "openFAD", "UTT", Guid.NewGuid().ToString("N"));
        var restartRequired = false;

        try
        {
            Directory.CreateDirectory(tempRoot);
            var payloadZip = Path.Combine(tempRoot, "utt_data.zip");
            var webViewInstaller = Path.Combine(tempRoot, "MicrosoftEdgeWebView2RuntimeInstallerX64.exe");
            var vcRedistInstaller = Path.Combine(tempRoot, "VC_redist.x64.exe");
            var licenseFile = Path.Combine(tempRoot, "LICENSE.txt");

            ExtractResource(ZipResource, payloadZip);
            ExtractResource(WebViewResource, webViewInstaller);
            ExtractResource(VcRedistResource, vcRedistInstaller);
            ExtractResource(LicenseResource, licenseFile);

            restartRequired |= RunPrerequisite(webViewInstaller, "/silent /install", "Microsoft Edge WebView2 Runtime");
            restartRequired |= RunPrerequisite(vcRedistInstaller, "/install /quiet /norestart", "Microsoft Visual C++ Runtime");

            Directory.CreateDirectory(vst3Root);
            if (Directory.Exists(targetDir))
                Directory.Delete(targetDir, true);

            ZipFile.ExtractToDirectory(payloadZip, vst3Root);
            var modulePath = Path.Combine(targetDir, "Contents", "x86_64-win", "UTT.vst3");
            if (!File.Exists(modulePath))
                throw new InvalidOperationException("The extracted VST3 bundle is incomplete.");
            File.Copy(licenseFile, Path.Combine(vst3Root, "UTT_LICENSE.txt"), true);

            if (!quiet)
            {
                var completion = "Installation complete. Restart your DAW before inserting UTT.";
                if (restartRequired)
                    completion += "\n\nWindows reported that a restart may be required after installing prerequisites.";
                MessageBox.Show(completion, "UTT Installer", MessageBoxButtons.OK, MessageBoxIcon.Information);
            }

            return 0;
        }
        catch (Exception ex)
        {
            if (!quiet)
                MessageBox.Show("Installation failed:\n" + ex.Message,
                    "UTT Installer Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            return 1;
        }
        finally
        {
            try
            {
                if (Directory.Exists(tempRoot))
                    Directory.Delete(tempRoot, true);
            }
            catch
            {
                // A pending prerequisite process can briefly retain its extracted file.
            }
        }
    }

    private static void ExtractResource(string resourceName, string destination)
    {
        var assembly = Assembly.GetExecutingAssembly();
        using (var input = assembly.GetManifestResourceStream(resourceName))
        {
            if (input == null)
                throw new InvalidOperationException("Missing embedded installer payload: " + resourceName);

            using (var output = new FileStream(destination, FileMode.Create, FileAccess.Write, FileShare.None))
                input.CopyTo(output);
        }
    }

    private static bool RunPrerequisite(string executable, string arguments, string displayName)
    {
        var startInfo = new ProcessStartInfo
        {
            FileName = executable,
            Arguments = arguments,
            UseShellExecute = false,
            CreateNoWindow = true,
            WorkingDirectory = Path.GetDirectoryName(executable)
        };

        using (var process = Process.Start(startInfo))
        {
            if (process == null)
                throw new InvalidOperationException("Unable to start " + displayName + ".");

            process.WaitForExit();
            var exitCode = process.ExitCode;
            if (exitCode == 0 || exitCode == 1638)
                return false;
            if (exitCode == 1641 || exitCode == 3010)
                return true;

            throw new InvalidOperationException(displayName + " returned exit code " + exitCode + ".");
        }
    }
}
