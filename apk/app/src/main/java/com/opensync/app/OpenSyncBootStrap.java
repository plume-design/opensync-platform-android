package com.opensync.app;

import android.app.Activity;
import android.content.Context;
import android.content.res.AssetManager;
import android.os.Handler;
import android.util.Log;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import java.io.*;
import java.util.zip.*;
import java.nio.*;
import java.nio.file.*;
import java.nio.file.attribute.*;
import java.util.*;


import org.apache.commons.io.FileUtils;

public class OpenSyncBootStrap {
    private static final String TAG = "OpenSync BootStrap";

    private final Context context;
    private String appRunFilesDir = "";
    private String appRunCacheDir = "";
    private final HashMap<String, String[]> processArgvHash = new HashMap<>();

    public OpenSyncBootStrap(Context context) {
        this.context = context;
        appRunFilesDir = context.getFilesDir().getAbsolutePath() + "/";
        appRunCacheDir = context.getCacheDir().getAbsolutePath() + "/";

        Log.i(TAG, "APP running dir + files: " + appRunFilesDir);
        Log.i(TAG, "APP running dir + cache: " + appRunCacheDir);

        /* OVSDB bootstrap command */
        String[] ovsdbServerValue = {
                                appRunFilesDir + "bin/ovsdb-server",
                "--remote=punix:" + appRunCacheDir + "openvswitch/db.sock",
                "--remote=db:Open_vSwitch,Open_vSwitch,manager_options",
                "--private-key=db:Open_vSwitch,SSL,private_key",
                "--certificate=db:Open_vSwitch,SSL,certificate",
                "--ca-cert=db:Open_vSwitch,SSL,ca_cert",
                "--pidfile=" + appRunCacheDir + "ovsdb-server.pid ",
                appRunCacheDir + "openvswitch/conf.db"
        };

        /* OpenSync bootstrap command */
        String[] dmValue = {
                                appRunFilesDir + "opensync/bin/dm",
        };

        processArgvHash.put("ovsdb-server", ovsdbServerValue);
        processArgvHash.put("dm", dmValue);
    }

    public void StartProcessHelper() {
        for (Map.Entry<String, String[]> entry : processArgvHash.entrySet()) {
            String key = entry.getKey();
            String[] value = entry.getValue();
            executeBinaryAsync(key, value);
        }
    }

    public void preStart() {
        AssetManager assetManager = context.getAssets();
        /* Copy assets to app private storage */
        unzipOpenSyncNative(assetManager);
        /* Copy certs */
        copyCertstoCache();
        /* Copy OVSDB file */
        copyOVStoCache();
    }

    private void copyCertstoCache() {
        try {
            String srcDir = appRunFilesDir + "opensync/certs";
            String destDir = appRunCacheDir + "certs";

            FileUtils.copyDirectory(new File(srcDir), new File(destDir));
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private void copyOVStoCache() {
        try {
            java.nio.file.Path srcPath = Paths.get(appRunFilesDir + "opensync/etc/conf.db.bck");
            FileSystem fileSystem = FileSystems.getDefault();
            Path directory = fileSystem.getPath(appRunCacheDir + "openvswitch");

            try {
                Set<PosixFilePermission> perms = PosixFilePermissions.fromString("rwxrwxr--");
                FileAttribute<Set<PosixFilePermission>> attr = PosixFilePermissions.asFileAttribute(perms);
                Files.createDirectory(directory, attr);
            } catch (FileAlreadyExistsException x) {
                // ignore
            } catch (IOException e) {
                e.printStackTrace();
            }

            java.nio.file.Path dstPath = Paths.get(appRunCacheDir + "openvswitch/conf.db");
            Files.copy(srcPath, dstPath, StandardCopyOption.REPLACE_EXISTING);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private void copyFile(InputStream in, OutputStream out) throws IOException {
        byte[] buffer = new byte[1024];
        int read;
        while ((read = in.read(buffer)) != -1) {
            out.write(buffer, 0, read);
        }
    }

    private static void unzip(File zipFile, File targetDirectory) throws IOException {
        ZipInputStream zis = new ZipInputStream(
                new BufferedInputStream(new FileInputStream(zipFile)));
        try {
            ZipEntry ze;
            int count;
            byte[] buffer = new byte[8192];
            while ((ze = zis.getNextEntry()) != null) {
                File file = new File(targetDirectory, ze.getName());
                File dir = ze.isDirectory() ? file : file.getParentFile();
                if (!dir.isDirectory() && !dir.mkdirs())
                    throw new FileNotFoundException("Failed to ensure directory: " +
                            dir.getAbsolutePath());
                if (ze.isDirectory())
                    continue;
                FileOutputStream fout = new FileOutputStream(file);
                try {
                    while ((count = zis.read(buffer)) != -1)
                        fout.write(buffer, 0, count);
                } finally {
                    fout.close();
                }
                /* if time should be restored as well
                long time = ze.getTime();
                if (time > 0)
                    file.setLastModified(time);
                */
                file.setExecutable(true);
            }
        } finally {
            zis.close();
        }
    }

    private void unzipOpenSyncNative(AssetManager assetManager) {
        try {
            InputStream in = assetManager.open("opensync.zip");
            File outFile = new File(appRunFilesDir, "opensync.zip");

            OutputStream out = new FileOutputStream(outFile);
            copyFile(in, out);
            in.close();
            out.flush();
            out.close();

            unzip(outFile, new File(appRunFilesDir));
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private void executeBinaryAsync(String Path, String[] ProcessArgv) {
        Thread thread = new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    ProcessBuilder processBuilder = new ProcessBuilder(ProcessArgv);

                    // Get the environment of the process
                    Map<String, String> environment = processBuilder.environment();
                    // Set the LD_LIBRARY_PATH environment variable
                    environment.put("LD_LIBRARY_PATH", appRunFilesDir + "lib:" + appRunFilesDir + "opensync/lib");
                    environment.put("PATH", appRunFilesDir + "bin:" + appRunFilesDir + "bin:$PATH");
                    environment.put("TMPDIR", appRunCacheDir);

                    processBuilder.environment().putAll(environment);

                    processBuilder.directory(new File(appRunFilesDir));

                    processBuilder.redirectErrorStream(true);
                    List<String> fullCommand = processBuilder.command();
                    Process process = processBuilder.start();
                    Log.d(TAG, String.join(" ", fullCommand));

                    BufferedReader reader = new BufferedReader(new InputStreamReader(process.getInputStream()));
                    BufferedReader errorReader = new BufferedReader(new InputStreamReader(process.getErrorStream()));

                    String line;
                    while ((line = reader.readLine()) != null) {
                        Log.d(TAG, line);
                    }

                    while ((line = errorReader.readLine()) != null) {
                        Log.e("BinaryError", line);
                    }

                    int exitCode = process.waitFor();
                    Log.d("BinaryExitCode", String.valueOf(exitCode));
                } catch (IOException | InterruptedException e) {
                    e.printStackTrace();
                }
            }
        });

        Log.i(TAG + " Thread Start", Path + " " + Arrays.toString(ProcessArgv));
        thread.start();
    }
}
