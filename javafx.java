// Copyright 2025 David Lareau. This source code form is subject to the terms of the Mozilla Public License 2.0.
// javac --module-path /var/lib/snapd/snap/openjfx/current/sdk/lib --add-modules javafx.controls,javafx.fxml -cp .:jna-5.18.0.jar:jna-jpms-5.18.0.jar javafx.java -d java_out
// java --module-path /var/lib/snapd/snap/openjfx/current/sdk/lib --add-modules javafx.controls,javafx.fxml --enable-native-access=javafx.graphics -Dglass.gtk.uiScale=1.5 -cp java_out:jna-5.18.0.jar:jna-jpms-5.18.0.jar javafx
// note: openjfx was a pain to install. I ended up installing snapd from AUR, then sudo systemctl start snapd, then sudo snap install openjfx

import javafx.application.Application; import javafx.fxml.*; import javafx.stage.*;
import javafx.scene.*; import javafx.scene.control.*; import javafx.scene.image.*; import javafx.scene.input.*; import javafx.scene.layout.*; import javafx.event.*;
import javafx.geometry.*; import javafx.stage.FileChooser.*;

import java.util.*; import java.io.*;

import com.goterl.lazysodium.*; import com.goterl.lazysodium.interfaces.*;

import java.util.Random;
import java.text.SimpleDateFormat;
import java.nio.charset.StandardCharsets;

public class javafx extends Application { public static void main(String[] args) { launch(args); }

  static void show_error(String message) { Alert alert = new Alert(Alert.AlertType.ERROR); alert.setTitle("Error"); alert.setContentText(message); alert.show(); }
  static void show_warning(String message) { Alert alert = new Alert(Alert.AlertType.WARNING); alert.setTitle("Warning"); alert.setContentText(message); alert.show(); }
    
  @Override public void start(Stage stage) { try {
    FXMLLoader loader = new FXMLLoader(new File("javafx.fxml").toURI().toURL()); Parent root = loader.load(); javafx controller = loader.<javafx>getController();
    javafx.window = stage;
    javafx.sodium = new SodiumJava();
    { Image hide_icon = new Image("file:password-hide.png"); Image show_icon = new Image("file:password-show.png"); controller.set_icons(hide_icon, show_icon); }
    Scene scene = new Scene(root); stage.setTitle("Vault"); stage.setScene(scene); stage.show();
  } catch(Exception e) { e.printStackTrace(); show_error(e.toString()); } }

  static Window window;
  static SodiumJava sodium;
  static String folder = "/home/flux/_/files/vault/";
  
  @FXML private TextField id;
  @FXML private TextField url;
  @FXML private PasswordField username; @FXML private TextField username_viz; @FXML private ImageView username_viz_icon;
  @FXML private PasswordField password; @FXML private TextField password_viz; @FXML private ImageView password_viz_icon;
  @FXML private TextArea notes;

  @FXML void on_copy_url(ActionEvent event) { ClipboardContent content = new ClipboardContent(); content.putString(url.getText()); Clipboard.getSystemClipboard().setContent(content); }
  @FXML void on_copy_username(ActionEvent event) { ClipboardContent content = new ClipboardContent(); content.putString(username.getText()); Clipboard.getSystemClipboard().setContent(content); }
  @FXML void on_copy_password(ActionEvent event) { ClipboardContent content = new ClipboardContent(); content.putString(password.getText()); Clipboard.getSystemClipboard().setContent(content); }

  void decode(File file) {
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

  byte [] decode_(File file, String passphrase) throws IOException {
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
    String vault_passphrase_str = passphrase == null? showPasswordDialog() : passphrase; if(vault_passphrase_str == null) return null;
    byte [] vault_passphrase = vault_passphrase_str.getBytes(StandardCharsets.UTF_8);
    byte [] key = new byte[AEAD.XCHACHA20POLY1305_IETF_KEYBYTES];
    if(sodium.crypto_pwhash(key, AEAD.XCHACHA20POLY1305_IETF_KEYBYTES, vault_passphrase, vault_passphrase.length, salt, algo_p1, new com.sun.jna.NativeLong(algo_p2), (int)algo) != 0) throw new RuntimeException("failed to derive key");

    // decrypt
    byte [] decrypted = new byte[(int)(encrypted_n - AEAD.XCHACHA20POLY1305_IETF_ABYTES)];
    long [] decrypted_n_ptr = new long[1];
    if(sodium.crypto_aead_xchacha20poly1305_ietf_decrypt(decrypted, decrypted_n_ptr, null, encrypted, encrypted_n, null, 0, nonce, key) != 0) throw new RuntimeException("failed to decrypt");
    
    return decrypted;
  }

  void encode() { try {
    String vault_passphrase_str = showPasswordDialog(); if(vault_passphrase_str == null) return;
    
    // open an arbitrary existing file and test passphrase on it, to ensure passphrase used for encoding has no typo that would lock us in
    File[] files = new File(folder).listFiles();
    if(files.length == 0) {
      Alert alert = new Alert(Alert.AlertType.WARNING); alert.setTitle("Warning"); alert.setContentText("This is a new folder, so we won't ensure the passphrase has no typos. Please double check the file."); alert.show();
    } else  {
      try {
        File file = files[new Random().nextInt(files.length)];      
        decode_(file, vault_passphrase_str);
      } catch(Exception e) {
        Alert alert = new Alert(Alert.AlertType.ERROR); alert.setTitle("Error"); alert.setContentText("Key didn't work with a random file from vault. Typo likely. Try saving again."); alert.show();
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

  // create a text input dialog
  String showPasswordDialog() {
    PasswordField pwd = new PasswordField(); HBox content = new HBox(); content.setAlignment(Pos.CENTER); content.getChildren().addAll(pwd); HBox.setHgrow(pwd, Priority.ALWAYS);
    Dialog<String> dialog = new Dialog<>(); dialog.initOwner(window); dialog.setTitle("Vault Passphrase"); dialog.getDialogPane().getButtonTypes().addAll(ButtonType.OK, ButtonType.CANCEL); dialog.getDialogPane().setContent(content);
    dialog.setResultConverter(dialogButton -> { if (dialogButton == ButtonType.OK) { return pwd.getText(); } return null; });
    Optional<String> result = dialog.showAndWait(); if(result.isPresent()) { return result.get(); } return null;
  }

  @FXML void on_open(ActionEvent event) {
    FileChooser chooser = new FileChooser(); chooser.setInitialDirectory(new File(folder)); chooser.setTitle("Open Resource File"); chooser.getExtensionFilters().addAll(new ExtensionFilter("Encrypted Files", "*.enc"));
    File file = chooser.showOpenDialog(window); if (file != null) {
      decode(file);
    }
  }

  @FXML void on_save(ActionEvent event) {
  }

  private Image hide_icon, show_icon;
  void set_icons(Image hide_icon, Image show_icon) {
    this.hide_icon = hide_icon;
    this.show_icon = show_icon;
    this.username_viz_icon.setImage(hide_icon);
    this.password_viz_icon.setImage(hide_icon);
    // set to hidden
    this.username.toFront();
    this.username_viz_icon.toFront();
    this.password.toFront();
    this.password_viz_icon.toFront();
    // ensure both field are in sync
    this.username.textProperty().bindBidirectional(this.username_viz.textProperty());
    this.password.textProperty().bindBidirectional(this.password_viz.textProperty());
  }
  @FXML void on_username_viz(MouseEvent event) {
    boolean reveal = this.username_viz_icon.getImage() == this.hide_icon;
    if(reveal) {
      this.username_viz.toFront();
      this.username_viz_icon.toFront();
      this.username_viz_icon.setImage(this.show_icon);
    } else {
      this.username.toFront();
      this.username_viz_icon.toFront();
      this.username_viz_icon.setImage(this.hide_icon);
    }
  }
  @FXML void on_password_viz(MouseEvent event) {
    boolean reveal = this.password_viz_icon.getImage() == this.hide_icon;
    if(reveal) {
      this.password_viz.toFront();
      this.password_viz_icon.toFront();
      this.password_viz_icon.setImage(this.show_icon);
    } else {
      this.password.toFront();
      this.password_viz_icon.toFront();
      this.password_viz_icon.setImage(this.hide_icon);
    }
  }
}