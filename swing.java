// Copyright 2025 David Lareau. This source code form is subject to the terms of the Mozilla Public License 2.0.
// javac -Xlint:deprecation -cp .:jna-5.18.0.jar:jna-jpms-5.18.0.jar swing.java
// java -Dsun.java2d.uiScale.enabled=true -Dsun.java2d.uiScale=2 -Djava.awt.headless=false --enable-native-access=ALL-UNNAMED -cp .:jna-5.18.0.jar:jna-jpms-5.18.0.jar swing
//
// find . -name "*.class" -exec rm {} \;
// find . -name "*.java" -exec grep SEARCHITEM {} /dev/null \;
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
import com.goterl.lazysodium.*;
import com.goterl.lazysodium.interfaces.*;

public class swing {

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
      // open file and read header
      InputStream in = new FileInputStream(file);
      DataInputStream din = new DataInputStream(in);
      long algo = din.readLong();
      long algo_p1 = din.readLong();
      long algo_p2 = din.readLong();
      byte [] salt = new byte[PwHash.SALTBYTES]; din.readFully(salt);
      byte [] nonce = new byte[AEAD.XCHACHA20POLY1305_IETF_NPUBBYTES]; din.readFully(nonce);
      long encrypted_n = din.readLong();
      if(encrypted_n <= AEAD.XCHACHA20POLY1305_IETF_ABYTES) throw new RuntimeException("encrypted length too short");

      String vault_passphrase = showPasswordDialog("Vault Passphrase"); if(vault_passphrase == null) return;
    } catch(IOException e) { throw new RuntimeException(e); }
  }

  static void encode() {
    System.out.println("encode");
  }

  public static void main(String[] args) {
    //SodiumLibrary.setLibraryPath("/usr/lib/libsodium.so"); System.out.println(SodiumLibrary.libsodiumVersionString());
    sodium = new SodiumJava();
    JTextField id = new JTextField(); JTextField url = new JTextField(); JTextField username = new JTextField(); JTextField password = new JTextField(); JTextArea notes = new JTextArea();
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
        JButton open = new JButton("Open", UIManager.getIcon("FileView.directoryIcon")); open.addActionListener(new ActionListener() { public void actionPerformed(ActionEvent e) { JFileChooser chooser = new JFileChooser("/home/flux/_/files/vault/"); FileNameExtensionFilter filter = new FileNameExtensionFilter("Encrypted", "enc"); chooser.setFileFilter(filter); int returnVal = chooser.showOpenDialog(frame); if(returnVal == JFileChooser.APPROVE_OPTION) { decode(chooser.getSelectedFile()); } }}); this.add(open);
      }}, BorderLayout.SOUTH);
    }});
    frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE); frame.pack(); frame.setVisible(true);
  }
}