// Copyright 2025 David Lareau. This source code form is subject to the terms of the Mozilla Public License 2.0.
// javac swing.java && java -Dsun.java2d.uiScale.enabled=true -Dsun.java2d.uiScale=2 -Djava.awt.headless=false swing
//
// To get list of built-in icons: SortedSet<String> set = new TreeSet<>(); for(Object o : UIManager.getDefaults().keySet()) { set.add(o.toString()); } for(String s : set) { if(s.endsWith("Icon")) System.out.println(s); }
//
// I'm not super invested in this implementation because JavaFX has replaced Swing. I'm mostly writting this one to get back on the saddle before doing the JavaFX implementation.
import javax.swing.*;
import java.util.*;
import java.awt.*;

public class swing {
  public static void main(String[] args) {
    JFrame frame = new JFrame("Vault"); //frame.setIconImage(UIManager.getIcon("FileView.directoryIcon").getImage());
    frame.setContentPane(new JPanel(new BorderLayout()) {{
      this.add(new JPanel() {{
        this.setLayout(new BoxLayout(this, BoxLayout.Y_AXIS));
        this.add(new JPanel() {{
          this.setLayout(new BoxLayout(this, BoxLayout.X_AXIS));
          this.add(new JLabel("ID"));
          this.add(new JTextField());
        }});
        this.add(new JPanel() {{
          this.setLayout(new BoxLayout(this, BoxLayout.X_AXIS));
          this.add(new JLabel("URL", UIManager.getIcon("FileChooser.homeFolderIcon"), JLabel.LEFT));
          this.add(new JTextField());
          this.add(new JButton("Copy", UIManager.getIcon("FileView.floppyDriveIcon")));
        }});
        this.add(new JPanel() {{
          this.setLayout(new BoxLayout(this, BoxLayout.X_AXIS));
          this.add(new JLabel("Username"));
          this.add(new JTextField());
          this.add(new JButton("Copy", UIManager.getIcon("FileView.floppyDriveIcon")));
        }});
        this.add(new JPanel() {{
          this.setLayout(new BoxLayout(this, BoxLayout.X_AXIS));
          this.add(new JLabel("Password"));
          this.add(new JTextField());
          this.add(new JButton("Copy", UIManager.getIcon("FileView.floppyDriveIcon")));
        }});
        this.add(new JPanel() {{
          this.setLayout(new BoxLayout(this, BoxLayout.X_AXIS));
          this.add(new JLabel("Notes"));
          this.add(new JTextArea());
        }});
      }}, BorderLayout.CENTER);
      this.add(new JPanel() {{
        this.setLayout(new BoxLayout(this, BoxLayout.X_AXIS));
        this.add(new JButton("Save", UIManager.getIcon("FileView.floppyDriveIcon")));
        this.add(new JButton("Open", UIManager.getIcon("FileView.directoryIcon")));
      }}, BorderLayout.SOUTH);
    }});
    frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE); frame.pack(); frame.setVisible(true);
  }
}