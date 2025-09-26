// Copyright 2025 David Lareau. This source code form is subject to the terms of the Mozilla Public License 2.0.
// javac -Xlint:deprecation -cp .:jna-5.18.0.jar:jna-jpms-5.18.0.jar swing.java
// java -Dsun.java2d.uiScale.enabled=true -Dsun.java2d.uiScale=2 -Djava.awt.headless=false --enable-native-access=ALL-UNNAMED -cp .:jna-5.18.0.jar:jna-jpms-5.18.0.jar swing
//
// find . -name "*.class" -exec rm {} \;
// find . -name "*.java" -exec grep -i SEARCHITEM {} /dev/null \;
//
// To get list of built-in icons: SortedSet<String> set = new TreeSet<>(); for(Object o : UIManager.getDefaults().keySet()) { set.add(o.toString()); } for(String s : set) { if(s.endsWith("Icon")) System.out.println(s); }
//
// I'm not super invested in this implementation because JavaFX has replaced Swing. I'm mostly writting this one to get back on the saddle before doing the JavaFX implementation.
import javax.swing.*;
import java.util.*;
import java.awt.*;
import java.awt.event.*;
import javax.swing.filechooser.*;
import java.io.*;
import java.nio.charset.StandardCharsets;
import com.goterl.lazysodium.*;
import com.goterl.lazysodium.interfaces.*;
import java.util.Random;
import java.text.SimpleDateFormat;

public class swing {

  static String folder = "/home/flux/_/files/vault/";
  
  static JTextField id, url, username, password; static JTextArea notes;
  static SodiumJava sodium;

  static void clipboard(String s) { Toolkit.getDefaultToolkit().getSystemClipboard().setContents(new java.awt.datatransfer.StringSelection(s), null); }
  
  static String showPasswordDialog(String message) {
    JPanel panel = new JPanel(); JLabel label = new JLabel(message); JPasswordField pass = new JPasswordField(20); panel.add(label); panel.add(pass);
    String [] options = new String [] {"OK", "Cancel"}; int option = JOptionPane.showOptionDialog(null, panel, message, JOptionPane.NO_OPTION, JOptionPane.PLAIN_MESSAGE, null, options, options[0]);
    if(option == 0) { return new String(pass.getPassword()); } return null;
  }
  
  static void decode(File file) {
    try {
      byte [] decrypted = decode_(file, null); if(decrypted == null) return;
      // decrypted data is a utf8 \n separated file of id, url, username, password, notes for the rest of file
      BufferedReader reader = new BufferedReader(new CharArrayReader(new String(decrypted, StandardCharsets.UTF_8).toCharArray()));
      id.setText(reader.readLine());
      url.setText(reader.readLine());
      username.setText(reader.readLine());
      password.setText(reader.readLine());
      // read the rest (in a stupid way, I can't used decrypted_n because that's bytes, not utf8 char[])
      StringBuffer the_rest = new StringBuffer(); while(true) { int r = reader.read(); if(r == -1) break; the_rest.append((char)r); }
      notes.setText(the_rest.toString());
    } catch(IOException e) { throw new RuntimeException(e); }
  }

  static byte [] decode_(File file, String passphrase) throws IOException {
    // open file and read header
    DataInputStream din = new DataInputStream(new FileInputStream(file)); // keep in mind Java likes, in it's own word, 'machine-independent' byte order, which according to them is Big Endian, the loser of the endian war.
    long algo = Long.reverseBytes(din.readLong());
    long algo_p1 = Long.reverseBytes(din.readLong());
    long algo_p2 = Long.reverseBytes(din.readLong());
    byte [] salt = new byte[PwHash.SALTBYTES]; din.readFully(salt);
    byte [] nonce = new byte[AEAD.XCHACHA20POLY1305_IETF_NPUBBYTES]; din.readFully(nonce);
    long encrypted_n = Long.reverseBytes(din.readLong());
    if(encrypted_n <= AEAD.XCHACHA20POLY1305_IETF_ABYTES) throw new RuntimeException("encrypted length too short");
    byte [] encrypted = new byte[(int)encrypted_n]; din.readFully(encrypted);
    din.close();

    // ask for the passphrase and derive key from it
    String vault_passphrase_str = passphrase == null? showPasswordDialog("Vault Passphrase") : passphrase; if(vault_passphrase_str == null) return null;
    byte [] vault_passphrase = vault_passphrase_str.getBytes(StandardCharsets.UTF_8);
    byte [] key = new byte[AEAD.XCHACHA20POLY1305_IETF_KEYBYTES];
    if(sodium.crypto_pwhash(key, AEAD.XCHACHA20POLY1305_IETF_KEYBYTES, vault_passphrase, vault_passphrase.length, salt, algo_p1, new com.sun.jna.NativeLong(algo_p2), (int)algo) != 0) throw new RuntimeException("failed to derive key");

    // decrypt
    byte [] decrypted = new byte[(int)(encrypted_n - AEAD.XCHACHA20POLY1305_IETF_ABYTES)];
    long [] decrypted_n_ptr = new long[1];
    if(sodium.crypto_aead_xchacha20poly1305_ietf_decrypt(decrypted, decrypted_n_ptr, null, encrypted, encrypted_n, null, 0, nonce, key) != 0) throw new RuntimeException("failed to decrypt");
    
    return decrypted;
  }

  static void encode() { try {
    String vault_passphrase_str = showPasswordDialog("Vault Passphrase"); if(vault_passphrase_str == null) return;
    
    // open an arbitrary existing file and test passphrase on it, to ensure passphrase used for encoding has no typo that would lock us in
    File[] files = new File(folder).listFiles();
    if(files.length == 0) {
      JOptionPane.showMessageDialog(null, "This is a new folder, so we won't ensure the passphrase has no typos. Please double check the file.", "Warning", JOptionPane.WARNING_MESSAGE); 
    } else  {
      try {
        File file = files[new Random().nextInt(files.length)];      
        decode_(file, vault_passphrase_str);
      } catch(Exception e) {
        JOptionPane.showMessageDialog(null, "Key didn't work with a random file from vault. Typo likely. Try saving again.", "Error", JOptionPane.ERROR_MESSAGE);
        throw new RuntimeException(e);
      }
    }

    // derive key
    long algo = PwHash.Alg.PWHASH_ALG_ARGON2ID13.getValue();
    long algo_p1 = PwHash.ARGON2ID_OPSLIMIT_MODERATE;
    long algo_p2 = PwHash.ARGON2ID_MEMLIMIT_MODERATE;
    byte [] salt = new byte[PwHash.SALTBYTES]; sodium.randombytes_buf(salt, salt.length);
    byte [] vault_passphrase = vault_passphrase_str.getBytes(StandardCharsets.UTF_8);
    byte [] key = new byte[AEAD.XCHACHA20POLY1305_IETF_KEYBYTES];
    if(sodium.crypto_pwhash(key, AEAD.XCHACHA20POLY1305_IETF_KEYBYTES, vault_passphrase, vault_passphrase.length, salt, algo_p1, new com.sun.jna.NativeLong(algo_p2), (int)algo) != 0) throw new RuntimeException("failed to derive key");
    
    // encrypt
    StringBuilder s = new StringBuilder();
    s.append(id.getText()); s.append('\n');
    s.append(url.getText()); s.append('\n');
    s.append(username.getText()); s.append('\n');
    s.append(password.getText()); s.append('\n');
    s.append(notes.getText());
    byte [] src = s.toString().getBytes(StandardCharsets.UTF_8);
    byte [] nonce = new byte[AEAD.XCHACHA20POLY1305_IETF_NPUBBYTES]; sodium.randombytes_buf(nonce, nonce.length);
    byte [] dst = new byte[src.length+AEAD.XCHACHA20POLY1305_IETF_ABYTES];
    long [] dst_n_ptr = new long[1];
    if(sodium.crypto_aead_xchacha20poly1305_ietf_encrypt(dst, dst_n_ptr, src, src.length, null, 0, null, nonce, key) != 0) throw new RuntimeException("failed to encrypt"); long encrypted_n = dst_n_ptr[0];

    // sanitize entry for filename purpose, and add timestamp
    char [] chars = id.getText().toCharArray();
    for(int i = 0; i < chars.length; i++) {
      switch(chars[i]) { case '/': case '\\': case '?': case '%': case '*': case ':': case '|': case '"': case '<': case '>': case '.': case ',': case ';': case '=': case ' ': case '+': case '[': case ']': case '(': case ')': case '!': case '@': case '$':case '#': case '-': chars[i] = '_'; }
    }
    String filename = folder + new String(chars) + new SimpleDateFormat("yyyy-MM-dd_HH-mm-ss").format(new Date()) + ".enc";

    // write encrypted file
    OutputStream out = new FileOutputStream(filename);
    DataOutputStream dout = new DataOutputStream(out); // keep in mind Java likes, in it's own word, 'machine-independent' byte order, which according to them is Big Endian, the loser of the endian war.
    dout.writeLong(Long.reverseBytes(algo));
    dout.writeLong(Long.reverseBytes(algo_p1));
    dout.writeLong(Long.reverseBytes(algo_p2));
    dout.write(salt);
    dout.write(nonce);
    dout.writeLong(Long.reverseBytes(encrypted_n));
    dout.write(dst);
    dout.close();
    System.out.println("File save: " + filename);

  } catch(IOException e) { throw new RuntimeException(e); } }

  public static void main(String[] args) {
    //SodiumLibrary.setLibraryPath("/usr/lib/libsodium.so"); System.out.println(SodiumLibrary.libsodiumVersionString());
    sodium = new SodiumJava();
    id = new JTextField(); url = new JTextField(); username = new JTextField(); password = new JTextField(); notes = new JTextArea();
    JFrame frame = new JFrame("Vault"); //frame.setIconImage(UIManager.getIcon("FileView.directoryIcon").getImage());
    frame.setContentPane(new JPanel(new BorderLayout()) {{
      this.add(new JPanel() {{ this.setLayout(new BoxLayout(this, BoxLayout.Y_AXIS));
        this.add(new JPanel() {{ this.setLayout(new BoxLayout(this, BoxLayout.X_AXIS)); this.add(new JLabel("ID")); this.add(id); }});
        this.add(new JPanel() {{ this.setLayout(new BoxLayout(this, BoxLayout.X_AXIS)); this.add(new JLabel("URL", UIManager.getIcon("FileChooser.homeFolderIcon"), JLabel.LEFT)); this.add(url); JButton copy = new JButton("Copy", UIManager.getIcon("FileView.floppyDriveIcon")); copy.addActionListener(new ActionListener() { public void actionPerformed(ActionEvent e) { clipboard(url.getText()); }}); this.add(copy); }});
        this.add(new JPanel() {{ this.setLayout(new BoxLayout(this, BoxLayout.X_AXIS)); this.add(new JLabel("Username")); this.add(username); JButton copy = new JButton("Copy", UIManager.getIcon("FileView.floppyDriveIcon")); copy.addActionListener(new ActionListener() { public void actionPerformed(ActionEvent e) { clipboard(username.getText()); }}); this.add(copy); }});
        this.add(new JPanel() {{ this.setLayout(new BoxLayout(this, BoxLayout.X_AXIS)); this.add(new JLabel("Password")); this.add(password); JButton copy = new JButton("Copy", UIManager.getIcon("FileView.floppyDriveIcon")); copy.addActionListener(new ActionListener() { public void actionPerformed(ActionEvent e) { clipboard(password.getText()); }}); this.add(copy); }}); this.add(new JPanel() {{ this.setLayout(new BoxLayout(this, BoxLayout.X_AXIS)); this.add(new JLabel("Notes")); this.add(notes); }});
      }}, BorderLayout.CENTER);
      this.add(new JPanel() {{ this.setLayout(new BoxLayout(this, BoxLayout.X_AXIS));
        JButton save = new JButton("Save", UIManager.getIcon("FileView.floppyDriveIcon")); save.addActionListener(new ActionListener() { public void actionPerformed(ActionEvent e) { encode(); } }); this.add(save);
        JButton open = new JButton("Open", UIManager.getIcon("FileView.directoryIcon")); open.addActionListener(new ActionListener() { public void actionPerformed(ActionEvent e) { JFileChooser chooser = new JFileChooser(folder); FileNameExtensionFilter filter = new FileNameExtensionFilter("Encrypted", "enc"); chooser.setFileFilter(filter); int returnVal = chooser.showOpenDialog(frame); if(returnVal == JFileChooser.APPROVE_OPTION) { decode(chooser.getSelectedFile()); } }}); this.add(open);
      }}, BorderLayout.SOUTH);
    }});
    frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE); frame.pack(); frame.setVisible(true);
  }
}