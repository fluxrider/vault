import javafx.event.*; import javafx.fxml.*; import javafx.scene.control.*; import javafx.scene.image.*; import javafx.scene.input.*;

public class javafx_controller {

  @FXML private TextField id;
  @FXML private TextField url;
  @FXML private PasswordField username; @FXML private TextField username_viz; @FXML private ImageView username_viz_icon;
  @FXML private PasswordField password; @FXML private TextField password_viz; @FXML private ImageView password_viz_icon;
  @FXML private TextArea notes;

  @FXML void on_copy_url(ActionEvent event) { ClipboardContent content = new ClipboardContent(); content.putString(url.getText()); Clipboard.getSystemClipboard().setContent(content); }
  @FXML void on_copy_username(ActionEvent event) { ClipboardContent content = new ClipboardContent(); content.putString(username.getText()); Clipboard.getSystemClipboard().setContent(content); }
  @FXML void on_copy_password(ActionEvent event) { ClipboardContent content = new ClipboardContent(); content.putString(password.getText()); Clipboard.getSystemClipboard().setContent(content); }

  @FXML void on_open(ActionEvent event) {
    System.out.println("on open");
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
