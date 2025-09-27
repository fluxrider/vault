// Copyright 2025 David Lareau. This source code form is subject to the terms of the Mozilla Public License 2.0.
// javac --module-path /var/lib/snapd/snap/openjfx/current/sdk/lib --add-modules javafx.controls,javafx.fxml javafx.java -d java_out
// java --module-path /var/lib/snapd/snap/openjfx/current/sdk/lib --add-modules javafx.controls,javafx.fxml --enable-native-access=javafx.graphics -Dglass.gtk.uiScale=1.5 -cp java_out javafx
// note: openjfx was a pain to install. I ended up install snapd from AUR, then sudo systemctl start snapd, then sudo snap install openjfx

import javafx.application.Application;
import javafx.stage.*;
import javafx.scene.*;
import javafx.scene.control.*;
import javafx.fxml.*;
import java.io.*;

public class javafx extends Application { public static void main(String[] args) { launch(args); }
    
  @Override public void start(Stage stage) { try {
    /*
    Label label = new Label("Hello, World!");
    Scene scene = new Scene(label, 300, 100);
    stage.setTitle("Hello, World (JavaFX)");
    stage.setScene(scene);
    stage.show();
    */
    
    FXMLLoader loader = new FXMLLoader(new File("javafx.fxml").toURI().toURL());
    Parent root = loader.load();
    Scene scene = new Scene(root);
    stage.setTitle("Vault");
    stage.setScene(scene);
    stage.show();
  } catch(Exception e) { e.printStackTrace(); Alert alert = new Alert(Alert.AlertType.ERROR); alert.setTitle("Error"); alert.setContentText(e.toString()); alert.show(); } }

  /* viz password example (https://stackoverflow.com/questions/60049990/how-do-i-show-contents-from-the-password-field-in-javafx-using-checkbox)
  import javafx.application.Application;
  import javafx.geometry.Insets;
  import javafx.geometry.Pos;
  import javafx.scene.Scene;
  import javafx.scene.control.CheckBox;
  import javafx.scene.control.PasswordField;
  import javafx.scene.control.TextField;
  import javafx.scene.image.Image;
  import javafx.scene.image.ImageView;
  import javafx.scene.layout.HBox;
  import javafx.scene.layout.StackPane;
  import javafx.scene.layout.VBox;
  import javafx.stage.Stage;
  Image image = new Image("https://previews.123rf.com/images/andrerosi/andrerosi1905/andrerosi190500216/123158287-eye-icon-vector-look-and-vision-icon-eye-vector-icon.jpg");
      StackPane createPasswordFieldWithEye()
    {
        PasswordField passwordField = new PasswordField();
        passwordField.setPrefHeight(50);
        TextField textField = new TextField();
        passwordField.textProperty().bindBidirectional(textField.textProperty());
        textField.setPrefHeight(50);
        ImageView imageView = new ImageView(image);
        imageView.setFitHeight(32);
        imageView.setFitWidth(32);
        StackPane.setMargin(imageView, new Insets(0, 10, 0, 0));
        StackPane.setAlignment(imageView, Pos.CENTER_RIGHT);
        imageView.setOnMousePressed((event) -> {
            textField.toFront();
            imageView.toFront();
        });

        imageView.setOnMouseReleased((event) -> {
            passwordField.toFront();
            imageView.toFront();
        });

        StackPane root = new StackPane(textField, passwordField, imageView);

        return root;
    }
    */
    
}