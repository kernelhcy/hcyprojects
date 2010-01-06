/*
 * Christopher Deckers (chrriis@nextencia.net)
 * http://www.nextencia.net
 * 
 * See the file "readme.txt" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
package chrriis.udoc.model.processor.binary;

import java.awt.Dimension;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.File;

import javax.swing.JButton;
import javax.swing.JFileChooser;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JTextField;
import javax.swing.event.DocumentEvent;
import javax.swing.event.DocumentListener;
import javax.swing.filechooser.FileFilter;

import chrriis.udoc.model.Util;

public class BinaryParametersComponent extends JPanel {

  protected static JFileChooser fileChooser;

  public BinaryParametersComponent(final BinaryClassProcessor classProcessor) {
    GridBagLayout gridBag = new GridBagLayout();
    setLayout(gridBag);
    GridBagConstraints cons = new GridBagConstraints();
    cons.gridx = 0;
    cons.gridy = 0;
    JLabel classPathLabel = new JLabel("Classpath: ");
    gridBag.setConstraints(classPathLabel, cons);
    add(classPathLabel);
    cons.gridx++;
    cons.fill = GridBagConstraints.HORIZONTAL;
    cons.weightx = 1.0;
    final JTextField classPathTextField = new JTextField(classProcessor.getClassPath(), 24);
    gridBag.setConstraints(classPathTextField, cons);
    add(classPathTextField);
    cons.gridx++;
    cons.fill = GridBagConstraints.NONE;
    cons.weightx = 0.0;
    JButton browseButton = new JButton("Browse...");
    gridBag.setConstraints(browseButton, cons);
    add(browseButton);
    browseButton.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        if(fileChooser == null) {
          fileChooser = new JFileChooser();
          fileChooser.setFileSelectionMode(JFileChooser.FILES_AND_DIRECTORIES);
          fileChooser.setMultiSelectionEnabled(true);
          fileChooser.setFileFilter(new FileFilter() {
            public boolean accept(File f) {
              return f.isDirectory() || f.getName().endsWith(".zip") || f.getName().endsWith(".jar");
            }
            public String getDescription() {
              return "*.zip, *.jar, class directory";
            }
          });
        }
        if(fileChooser.showOpenDialog(BinaryParametersComponent.this) == JFileChooser.APPROVE_OPTION) {
          File[] selectedFiles = fileChooser.getSelectedFiles();
          StringBuffer pathSB = new StringBuffer();
          String pathSeparator = Util.getPathSeparator();
          for(int i=0; i<selectedFiles.length; i++) {
            if(i > 0) {
              pathSB.append(pathSeparator);
            }
            pathSB.append(selectedFiles[i].getAbsolutePath());
          }
          classPathTextField.setText(pathSB.toString());
        }
      }
    });
    classPathTextField.getDocument().addDocumentListener(new DocumentListener() {
      public void changedUpdate(DocumentEvent e) {
        classProcessor.setClassPath(classPathTextField.getText());
      }
      public void insertUpdate(DocumentEvent e) {
        classProcessor.setClassPath(classPathTextField.getText());
      }
      public void removeUpdate(DocumentEvent e) {
        classProcessor.setClassPath(classPathTextField.getText());
      }
    });
    setPreferredSize(new Dimension(500, getPreferredSize().height));
  }

}
