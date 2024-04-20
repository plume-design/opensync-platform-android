package com.opensync.app;

import android.content.*;
import android.os.Build;
import android.util.Log;

import java.io.*;
import java.nio.*;
import java.nio.file.*;
import java.nio.file.attribute.*;
import java.net.*;
import java.util.*;
import java.util.zip.*;

import org.apache.commons.compress.archivers.tar.*;


public class LogUtils {
    private static final String TAG = "OpenSync LogUtils";

    private static Context mContext;

    private Map<String, String[]> linuxCmds;  // fileName, Commands

    private int kernalLogNum = 20;
    private int kernalLogSize = 5 * 1024 * 1024;
    private int logTimeout = 10000;  // milliseconds
    private static final long MAX_LOGPULL_SIZE = 20 * 1024 * 1024;  // 20 MB
    private static final long MAX_FILE_SIZE = 1 * 1024 * 1024;

    private static String caFilePath;
    private static String cacheLogDir;
    private static String cacheDir;

    private static String uploadToken;
    private static String uploadLocation;

    private static String AppFilesDir;
    private static String installPrefix;
    private static String targetPathScript;

    private static final List<String> linuxFiles = List.of(
                "/proc/stat",
                "/proc/meminfo",
                "/proc/loadavg",
                "/proc/net/dev",
                "/proc/mtd",
                "/proc/sys/fs/file-nr"
            );

    public void setUploadToken(String token) {
        if (token != null) {
            uploadToken = token;
        }
    }

    public void setUploadLocation(String location) {
        if (location != null) {
            uploadLocation = location;
        }
    }

    public String getUploadToken() {
        return uploadToken;
    }

    public String getUploadLocation() {
        return uploadLocation;
    }

    public static String getDestPath() {
        return cacheLogDir;
    }

    private void addDefaultCommands() {
        linuxCmds.put("uname_-a", new String[] {"uname", "-a"});
        linuxCmds.put("uptime", new String[] {"uptime"});
        linuxCmds.put("date", new String[] {"date"});
        linuxCmds.put("ps_-A", new String[] {"ps", "-A"});
        linuxCmds.put("free", new String[] {"free"});
        linuxCmds.put("dmesg", new String[] {"dmesg"});
        linuxCmds.put("lspci", new String[] {"lspci"});
        linuxCmds.put("ifconfig_-a", new String[] {"ifconfig", "-a"});
        linuxCmds.put("ip_a", new String[] {"ip", "a"});
        linuxCmds.put("ip_-d_link_show", new String[] {"ip", "-d", "link", "show"});
        linuxCmds.put("lspci", new String[] {"lspci"});
        linuxCmds.put("ip_neigh_show", new String[] {"ip", "neigh", "show"});
        linuxCmds.put("ip_address", new String[] {"ip", "address"});
        linuxCmds.put("lsmod", new String[] {"lsmod"});
        linuxCmds.put("mount", new String[] {"mount"});
        linuxCmds.put("top_-n_1_-b", new String[] {"top", "-n", "1", "-b"});
        linuxCmds.put("netstat_-nep", new String[] {"netstat", "-nep"});
        linuxCmds.put("netstat_-nlp", new String[] {"netstat", "-nlp"});
        linuxCmds.put("netstat_-atp", new String[] {"netstat", "-atp"});
        linuxCmds.put("lsof", new String[] {"lsof"});

        // STB spcific
        linuxCmds.put("dumpsys", new String[] {"dumpsys"});
        linuxCmds.put("getprop", new String[] {"getprop"});
        linuxCmds.put("pm_list_packages", new String[] {"pm", "list", "packages"});
        linuxCmds.put("logcat", new String[] {"/system/bin/logcat", "-b", "all",
                                              "-v", "threadtime",
                                              "-v", "usec",
                                              "-v", "printable",
                                              "-D",
                                              "-f", cacheLogDir + "/logcat",
                                              "-r" + Integer.toString(kernalLogSize),
                                              "-n" + Integer.toString(kernalLogNum),
                                              "-t", Integer.toString(logTimeout)
                                             });


        //TODO: Support in Phase 2
        //linuxCmds.put("iptables_-L_-v_-n", new String[]{"iptables", "-L", "-v", "-n"});
        //linuxCmds.put("iptables_-t_nat_-L_-v_-n", new String[]{"iptables", "-t", "nat", "-L", "-v", "-n"});
        //linuxCmds.put("iptables_-t_mangle_-L_-v_-n", new String[]{"iptables", "-t", "mangle", "-L", "-v", "-n"});
        //linuxCmds.put("ip6tables_-L_-v_-n", new String[]{"ip6tables", "-L", "-v", "-n"});
        //linuxCmds.put("ip6tables_-t_nat_-L_-v_-n", new String[]{"ip6tables", "-t", "nat", "-L", "-v", "-n"});
        //linuxCmds.put("ip6tables_-t_mangle_-L_-v_-n", new String[]{"ip6tables", "-t", "mangle", "-L", "-v", "-n"});
        //linuxCmds.put("opensync.sh", new String[]{installPrefix+"/bin/dm", "--show-info"});
        //linuxCmds.put("ovsdb-client_dump", new String[]{targetPathScript+"ovsdb-client", "dump"});
        //linuxCmds.put("ovsdb-client_json_dump", new String[]{targetPathScript+"ovsdb-client", "-f", "json", "dump"});
        //linuxCmds.put("ovs-vsctl_version", new String[]{targetPathScript+"ovs-vsctl", "--version"});
        //linuxCmds.put("ovs-vsctl_show", new String[]{installPrefix+"/bin/ovs-vsctl", "show"});
        //linuxCmds.put("ovs-vsctl_list_bridge", new String[]{installPrefix+"/bin/ovs-vsctl", "list", "bridge"});
        //linuxCmds.put("ovs-vsctl_list_interface", new String[]{installPrefix+"/bin/ovs-vsctl", "list", "interface"});
        //linuxCmds.put("ovs-appctl_dpif_show", new String[]{installPrefix+"/bin/ovs-appctl", "dpif/show"});
        //linuxCmds.put("ovs-appctl_ovs_route_show", new String[]{installPrefix+"/bin/ovs-appctl", "ovs/route/show"});
    }

    public LogUtils(Context context) {
        this.mContext = context;
        this.linuxCmds = new HashMap<>();
        this.caFilePath = context.getCacheDir().getAbsolutePath() + "/certs";
        this.cacheLogDir = context.getCacheDir().getAbsolutePath() + "/logs";
        this.cacheDir = context.getCacheDir().getAbsolutePath();
        this.AppFilesDir = context.getFilesDir().getAbsolutePath();
        this.installPrefix = AppFilesDir + "/opensync";
        this.targetPathScript = AppFilesDir + "/bin/";

        addDefaultCommands();
        createCacheLogFolder();
    }

    public void doLogpull() {

        // Collect information
        boolean captureSuccess = captureLogs();
        if (captureSuccess == false) {
            Log.e(TAG, "Failed to get logs, doLogpull() failed ");
            return;
        }

        // Tar files
        tarFiles();

        // Upload file to AWS
        uploadTarGzFile();

        // Delete tar file
        deleteArchiveLogs();
    }

    private void deleteArchiveLogs() {
        File uploadedTGZ = new File(cacheDir + "/" + uploadToken);
        if (uploadedTGZ.exists()) {
            if (uploadedTGZ.delete()) {
                Log.d(TAG, "Existing output file removed: " + uploadToken);
            } else {
                Log.e(TAG, "Failed to remove existing output file: " + uploadToken);
            }
        }

        File dir = new File(cacheLogDir);
        if (dir.exists() && dir.isDirectory()) {
            File[] files = dir.listFiles();
            if (files != null) {
                for (File file : files) {
                    if (file.isFile()) {
                        if (!file.delete()) {
                            Log.d(TAG, "Failed to delete file: " + file.getName());
                        }
                    }
                }
            } else {
                Log.e(TAG, "Failed to list files in the directory.");
            }
        } else {
            Log.e(TAG, "Directory: " + cacheLogDir + " does not exist or is not a directory.");
        }
    }

    private void createCacheLogFolder() {
        Path cacheLogPath = Paths.get(cacheLogDir);

        try {
            if (Files.notExists(cacheLogPath)) {
                Files.createDirectories(cacheLogPath);

                String permissions = "rwxrwxr--";
                Set<PosixFilePermission> posixFilePermissions = PosixFilePermissions.fromString(permissions);

                Files.setPosixFilePermissions(cacheLogPath, posixFilePermissions);

                Log.i(TAG, "Folder created successfully: " + cacheLogPath.toAbsolutePath());
            } else {
                Log.d(TAG, "Folder already exists: " + cacheLogPath.toAbsolutePath());
            }
        } catch (Exception e) {
            Log.e(TAG, "Error creating or setting folder permissions: " + e.getMessage());
        }
    }

    // Prepare file and tar file
    private boolean captureLogs() {
        boolean ret = false;
        try {
            // Copy files
            for (String file : linuxFiles) {
                Path sourcePath = Paths.get(file);
                Path destinationFilePath = Paths.get(cacheLogDir, sourcePath.getFileName().toString());
                Log.d(TAG, "Gathering Linux Files: " + file);

                if (Files.exists(sourcePath)) {
                    try {
                        Files.copy(sourcePath, destinationFilePath, StandardCopyOption.REPLACE_EXISTING);
                    } catch (IOException e) {
                        e.printStackTrace();
                        Log.e(TAG, "Error copying file: " + file);
                    }
                } else {
                    Log.e(TAG, "File does not exist: " + file);
                }
            }


            for (String key : linuxCmds.keySet()) {
                String[] cmd = linuxCmds.get(key);
                String outputFileName = cacheLogDir + "/" + key;

                ProcessBuilder processBuilder = new ProcessBuilder(cmd);
                Process process = processBuilder.start();

                // Read command output
                try (BufferedReader reader = new BufferedReader(new InputStreamReader(process.getInputStream()));
                            BufferedWriter writer = new BufferedWriter(new FileWriter(outputFileName))) {
                    String line;
                    while ((line = reader.readLine()) != null) {
                        writer.write(line + "\n");
                    }
                }

                // Read error stream
                BufferedReader errorReader = new BufferedReader(new InputStreamReader(process.getErrorStream()));
                StringBuilder errorMessage = new StringBuilder();
                String errorLine;
                while ((errorLine = errorReader.readLine()) != null) {
                    errorMessage.append(errorLine).append("\n");
                }

                // Wait for the process to finish
                int exitCode = process.waitFor();
                if (exitCode == 0) {
                    ret = true;
                } else {
                    ret = false;
                    Log.e(TAG, "captureLogs failed with exit code: " + exitCode + "errMsg: " + errorMessage.toString());
                }
            }
        } catch (IOException | InterruptedException e) {
            ret = false;
            Log.e(TAG, "Error executing captureLogs", e);
        }
        return ret;
    }

    public static boolean tarFiles() {
        File tarFile = new File(cacheLogDir);
        try (FileOutputStream fos = new FileOutputStream(cacheDir + "/" + uploadToken);
                    BufferedOutputStream bos = new BufferedOutputStream(fos);
                    GZIPOutputStream gzOut = new GZIPOutputStream(bos);
                    TarArchiveOutputStream tarOut = new TarArchiveOutputStream(gzOut)) {

            long totalSize = 0;
            byte[] buf = new byte[1024];

            // Handle > 1 MB file
            File[] files = tarFile.listFiles();
            for (File child : files) {
                if (child.isFile() && (child.length() > MAX_FILE_SIZE)) {
                    totalSize += compressAndDeleteFile(child, tarOut);
                }
            }

            // Compress all the files
            File[] finalFiles = tarFile.listFiles();
            for (File child : finalFiles) {
                if (child.isFile()) {
                    totalSize += tarFile(child, tarOut, buf);
                    if (totalSize > MAX_LOGPULL_SIZE) {
                        Log.e(TAG, "Total size exceeds 20 MB. Compression stopped.");
                        break;
                    }
                }
            }

            Log.d(TAG, "Total Size of " + uploadToken + " is " + totalSize + " B.");
            tarOut.finish();
            return true;

        } catch (IOException e) {
            e.printStackTrace();
            return false;
        }
    }

    private static long compressAndDeleteFile(File file, TarArchiveOutputStream tarOut) throws IOException {
        try (FileInputStream fis = new FileInputStream(file);
                    BufferedInputStream bis = new BufferedInputStream(fis)) {
            String fileName = file.getName();
            File compressedFile = new File(file.getParent(), fileName + ".tar.gz");
            try (GZIPOutputStream gzipOut = new GZIPOutputStream(new FileOutputStream(compressedFile));
                        TarArchiveOutputStream individualTarOut = new TarArchiveOutputStream(gzipOut)) {
                TarArchiveEntry tae = new TarArchiveEntry(fileName);
                tae.setSize(file.length());
                individualTarOut.putArchiveEntry(tae);

                int len;
                byte[] buf = new byte[1024];
                while ((len = bis.read(buf)) > 0) {
                    individualTarOut.write(buf, 0, len);
                }

                individualTarOut.closeArchiveEntry();
                Log.d(TAG, "File processed: " + fileName);
                if (file.delete()) {
                    Log.d(TAG, "Delete original file: " + fileName);
                } else {
                    Log.e(TAG, "Failed to delete original file: " + fileName);
                }
                return compressedFile.length();
            }
        }
    }

    private static long tarFile(File file, TarArchiveOutputStream tarOut, byte[] buf) throws IOException {
        try (FileInputStream fis = new FileInputStream(file);
                    BufferedInputStream bis = new BufferedInputStream(fis)) {
            TarArchiveEntry tae = new TarArchiveEntry(file, file.getName());
            tae.setSize(file.length());
            tarOut.putArchiveEntry(tae);

            int len;
            while ((len = bis.read(buf)) > 0) {
                tarOut.write(buf, 0, len);
            }

            tarOut.closeArchiveEntry();
            return file.length();
        }
    }

    // Upload
    private static void uploadTarGzFile() {
        try {
            Path filePath = Paths.get(cacheDir +"/" + uploadToken);
            byte[] fileBytes = Files.readAllBytes(filePath);

            // Set connection
            URL url = new URL(uploadLocation);
            HttpURLConnection connection = (HttpURLConnection) url.openConnection();

            // Set SSL
            System.setProperty("javax.net.ssl.trustStore", caFilePath + "/opensync_ca.pem");
            System.setProperty("javax.net.ssl.trustStoreType", "X.509");

            connection.setRequestMethod("POST");
            connection.setRequestProperty("Accept", "*/*");
            connection.setRequestProperty("Content-Type", "application/octet-stream");
            connection.setRequestProperty("Content-Type", "multipart/form-data; boundary=------------------------a7381b8e4edcb37e");
            connection.setDoOutput(true);

            String boundary = "------------------------a7381b8e4edcb37e";
            try (OutputStream outputStream = connection.getOutputStream()) {
                outputStream.write(("--" + boundary + "\r\n").getBytes());
                outputStream.write(("Content-Disposition: form-data; name=\"filename\"; filename=\"" + filePath.toFile().getName() + "\"\r\n").getBytes());
                outputStream.write(("Content-Type: application/octet-stream\r\n\r\n").getBytes());
                outputStream.write(fileBytes);
                outputStream.write(("\r\n--" + boundary + "--\r\n").getBytes());
            } catch (IOException e) {
                Log.e(TAG, "Error writing to output stream: " + e.getMessage());
                e.printStackTrace();
            }
            int responseCode = connection.getResponseCode();
            String errorMessage = connection.getResponseMessage();
            if (responseCode == HttpURLConnection.HTTP_OK) {
                try (BufferedReader reader = new BufferedReader(new InputStreamReader(connection.getInputStream()))) {
                    String line;
                    while ((line = reader.readLine()) != null) {
                        Log.i(TAG, line);
                    }
                }
                Log.i(TAG, "File uploaded successfully. Response Code: " + responseCode);
            } else {
                Log.e(TAG, "File upload failed. Response Code: " + responseCode + " ErrorMsg: " + errorMessage);
            }
            connection.disconnect();
        } catch (Exception e) {
            Log.e(TAG, "Error during file upload: " + e.getMessage());
            e.printStackTrace();
        }
    }
}
