// Copyright 2025 David Lareau. This source code form is subject to the terms of the Mozilla Public License 2.0.
// javac --module-path /var/lib/snapd/snap/openjfx/current/sdk/lib --add-modules javafx.controls,javafx.fxml javafx*.java -d java_out
// java --module-path /var/lib/snapd/snap/openjfx/current/sdk/lib --add-modules javafx.controls,javafx.fxml --enable-native-access=javafx.graphics -Dglass.gtk.uiScale=1.5 -cp java_out javafx
// note: openjfx was a pain to install. I ended up installing snapd from AUR, then sudo systemctl start snapd, then sudo snap install openjfx

import javafx.application.Application; import javafx.stage.*; import javafx.scene.*; import javafx.scene.control.*; import javafx.scene.image.*; import javafx.fxml.*;
import java.io.*;

public class javafx extends Application { public static void main(String[] args) { launch(args); }
    
  @Override public void start(Stage stage) { try {
    FXMLLoader loader = new FXMLLoader(new File("javafx.fxml").toURI().toURL()); Parent root = loader.load(); javafx_controller controller = loader.<javafx_controller>getController();
    { Image hide_icon = new Image("file:password-hide.png"); Image show_icon = new Image("file:password-show.png"); controller.set_icons(hide_icon, show_icon); }
    Scene scene = new Scene(root); stage.setTitle("Vault"); stage.setScene(scene); stage.show();
  } catch(Exception e) { e.printStackTrace(); Alert alert = new Alert(Alert.AlertType.ERROR); alert.setTitle("Error"); alert.setContentText(e.toString()); alert.show(); } }
    
}