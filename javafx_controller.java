import javafx.event.ActionEvent;
import javafx.fxml.FXML;
import javafx.scene.control.PasswordField;
import javafx.scene.control.TextArea;
import javafx.scene.control.TextField;
import javafx.scene.image.*;
import javafx.scene.input.MouseEvent;

public class javafx_controller {

  private Image hide_icon, show_icon;
  
  void set_icons(Image hide_icon, Image show_icon) {
    this.hide_icon = hide_icon;
    this.show_icon = show_icon;
    this.username_viz_icon.setImage(hide_icon);
    this.password_viz_icon.setImage(hide_icon);
    // toggle to hidden
    textField.toFront();
    imageView.toFront();
  }

  @FXML private TextField id;

  @FXML private TextArea notes;

  @FXML private PasswordField password;

  @FXML private TextField password_viz;

  @FXML private ImageView password_viz_icon;

  @FXML private TextField url;

  @FXML private PasswordField username;

  @FXML private TextField username_viz;

  @FXML private ImageView username_viz_icon;
  
  @FXML void on_copy_password(ActionEvent event) {
  }

  @FXML void on_copy_url(ActionEvent event) {
  }

  @FXML void on_copy_username(ActionEvent event) {
  }

  @FXML void on_open(ActionEvent event) {
    System.out.println("on open");
  }

  @FXML void on_password_viz(MouseEvent event) {
  }

  @FXML void on_save(ActionEvent event) {
  }

  @FXML void on_username_viz(MouseEvent event) {
  }

}
