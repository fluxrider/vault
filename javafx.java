// Copyright 2025 David Lareau. This source code form is subject to the terms of the Mozilla Public License 2.0.
// javac --module-path /var/lib/snapd/snap/openjfx/current/sdk/lib --add-modules javafx.controls,javafx.fxml javafx.java -d java_out
// java --module-path /var/lib/snapd/snap/openjfx/current/sdk/lib --add-modules javafx.controls,javafx.fxml --enable-native-access=javafx.graphics -Dglass.gtk.uiScale=2.0 -cp java_out javafx
// note: openjfx was a pain to install. I ended up install snapd from AUR, then sudo systemctl start snapd, then sudo snap install openjfx

import javafx.application.Application;
import javafx.scene.Scene;
import javafx.scene.control.Label;
import javafx.stage.Stage;

public class javafx extends Application { public static void main(String[] args) { launch(args); }
    
  @Override public void start(Stage stage) {
    Label label = new Label("Hello, World!");
    Scene scene = new Scene(label, 300, 100);
    stage.setTitle("Hello, World (JavaFX)");
    stage.setScene(scene);
    stage.show();
  }

}