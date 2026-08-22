using System;
using System.IO;
using System.IO.Compression;
using System.Diagnostics;
using System.Windows.Forms;

class UTTInstaller
{
    [STAThread]
    static void Main()
    {
        string vst3Dir = Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.CommonProgramFiles),
            "Common Files\\VST3");
        string targetDir = Path.Combine(vst3Dir, "UTT.vst3");
        string exeDir = AppDomain.CurrentDomain.BaseDirectory;
        string zipPath = Path.Combine(exeDir, "utt_data.zip");

        if (!File.Exists(zipPath)) {
            MessageBox.Show("Cannot find utt_data.zip in:\n" + exeDir,
                "UTT Installer Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            return;
        }

        var result = MessageBox.Show(
            "UTT - Utility Toolbox by UnpureBloom\nVersion 0.2.0\n\n" +
            "Will install to:\n" + targetDir + "\n\nContinue?",
            "UTT Installer", MessageBoxButtons.YesNo, MessageBoxIcon.Question);

        if (result != DialogResult.Yes) return;

        try {
            if (Directory.Exists(targetDir))
                Directory.Delete(targetDir, true);

            ZipFile.ExtractToDirectory(zipPath, vst3Dir);

            string readme = Path.Combine(exeDir, "README.md");
            if (File.Exists(readme)) {
                File.Copy(readme, Path.Combine(vst3Dir, "UTT_README.md"), true);
                Process.Start(new ProcessStartInfo(readme) { UseShellExecute = true });
            }

            MessageBox.Show("Installation complete!\nPlease restart your DAW to load UTT.",
                "UTT Installer", MessageBoxButtons.OK, MessageBoxIcon.Information);
        }
        catch (Exception ex) {
            MessageBox.Show("Installation failed:\n" + ex.Message + "\n\nTry running as Administrator.",
                "UTT Installer Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
    }
}
