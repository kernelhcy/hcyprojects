/*
 * Christopher Deckers (chrriis@nextencia.net)
 * http://www.nextencia.net
 * 
 * See the file "readme.txt" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
package chrriis.udoc.model.processor.javadoc;

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

public class JavadocParametersComponent extends JPanel {

  protected static JFileChooser fileChooser;

  public JavadocParametersComponent(final JavadocClassProcessor classProcessor) {
    GridBagLayout gridBag = new GridBagLayout();
    setLayout(gridBag);
    GridBagConstraints cons = new GridBagConstraints();
    cons.gridx = 0;
    cons.gridy = 0;
    JLabel documentationLabel = new JLabel("Documentation: ");
    gridBag.setConstraints(documentationLabel, cons);
    add(documentationLabel);
    cons.gridx++;
    cons.fill = GridBagConstraints.HORIZONTAL;
    cons.weightx = 1.0;
    final JTextField documentationRootTextField = new JTextField(classProcessor.getDocumentationRoot(), 24);
    gridBag.setConstraints(documentationRootTextField, cons);
    add(documentationRootTextField);
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
          fileChooser.setFileSelectionMode(JFileChooser.DIRECTORIES_ONLY);
          fileChooser.setFileFilter(new FileFilter() {
            public boolean accept(File f) {
              return f.isDirectory();
            }
            public String getDescription() {
              return "Javadoc root directory";
            }
          });
        }
        if(fileChooser.showOpenDialog(JavadocParametersComponent.this) == JFileChooser.APPROVE_OPTION) {
          File f = fileChooser.getSelectedFile();
          documentationRootTextField.setText(f.getAbsolutePath());
        }
      }
    });
    documentationRootTextField.getDocument().addDocumentListener(new DocumentListener() {
      public void changedUpdate(DocumentEvent e) {
        classProcessor.setDocumentationRoot(documentationRootTextField.getText());
      }
      public void insertUpdate(DocumentEvent e) {
        classProcessor.setDocumentationRoot(documentationRootTextField.getText());
      }
      public void removeUpdate(DocumentEvent e) {
        classProcessor.setDocumentationRoot(documentationRootTextField.getText());
      }
    });
    setPreferredSize(new Dimension(500, getPreferredSize().height));
  }

}
