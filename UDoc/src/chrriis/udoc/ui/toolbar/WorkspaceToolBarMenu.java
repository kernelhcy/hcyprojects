/*
 * Christopher Deckers (chrriis@nextencia.net)
 * http://www.nextencia.net
 * 
 * See the file "readme.txt" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
package chrriis.udoc.ui.toolbar;

import java.awt.Color;
import java.awt.Component;
import java.awt.Dialog;
import java.awt.Frame;
import java.awt.Window;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.zip.Deflater;
import java.util.zip.DeflaterOutputStream;
import java.util.zip.InflaterInputStream;

import javax.swing.BoxLayout;
import javax.swing.JFileChooser;
import javax.swing.JPanel;
import javax.swing.SwingUtilities;
import javax.swing.filechooser.FileFilter;

import chrriis.udoc.ui.ClassPane;
import chrriis.udoc.ui.IconManager;
import chrriis.udoc.ui.Workspace;
import chrriis.udoc.ui.print.PrintPreviewDialog;

public class WorkspaceToolBarMenu extends ToolBarMenu {

  protected static final Color SELECTION_COLOR = new Color(218, 219, 255);

  protected ClassPane classPane;

  public WorkspaceToolBarMenu(ClassPane classPane) {
    super(IconManager.getIcon("tool_bar_on.gif"), IconManager.getIcon("tool_bar_off.gif"), classPane);
    this.classPane = classPane;
  }

  protected String getContentTitle() {
    return "Workspace";
  }

  protected Color getSelectionColor() {
    return SELECTION_COLOR;
  }

  protected JFileChooser fileChooser;

  protected void createFileChooser() {
    if(fileChooser == null) {
      fileChooser = new JFileChooser();
    }
  }
  
  protected Component createPopupMenuContent() {
    JPanel workspacePane = new JPanel();
    BoxLayout boxLayout = new BoxLayout(workspacePane, BoxLayout.Y_AXIS);
    workspacePane.setLayout(boxLayout);
    ToolBarMenuItem loadWorkspaceMenuItem = new ToolBarMenuItem("Load Workspace...");
    loadWorkspaceMenuItem.addActionListener(new ActionListener(){
      public void actionPerformed(ActionEvent e) {
        classPane.closeMenus();
        createFileChooser();
        fileChooser.setFileFilter(new FileFilter() {
          public boolean accept(File f) {
            return f.isDirectory() || f.getName().endsWith(".udc");
          }
          public String getDescription() {
            return "*.udc";
          }
        });
        if(fileChooser.showOpenDialog(WorkspaceToolBarMenu.this) == JFileChooser.APPROVE_OPTION) {
          File f = fileChooser.getSelectedFile();
          try {
            InputStream in = new BufferedInputStream(new InflaterInputStream(new FileInputStream(f)));
            ByteArrayOutputStream out = new ByteArrayOutputStream();
            byte[] bytes = new byte[1024];
            for(int count; (count=in.read(bytes)) >= 0; ) {
              out.write(bytes, 0, count);
            }
            Workspace.setXMLDescription(classPane, new String(out.toByteArray(), "UTF-8"));
            in.close();
          } catch(Exception ex) {
            try {
              InputStream in = new BufferedInputStream(new FileInputStream(f));
              ByteArrayOutputStream out = new ByteArrayOutputStream();
              byte[] bytes = new byte[1024];
              for(int count; (count=in.read(bytes)) >= 0; ) {
                out.write(bytes, 0, count);
              }
              Workspace.setXMLDescription(classPane, new String(out.toByteArray(), "UTF-8"));
              in.close();
            } catch(Exception ex2) {
              ex2.printStackTrace();
            }
          }
        }
      }
    });
    workspacePane.add(loadWorkspaceMenuItem);
    ToolBarMenuItem saveWorkspaceMenuItem = new ToolBarMenuItem("Save Workspace...");
    saveWorkspaceMenuItem.addActionListener(new ActionListener(){
      public void actionPerformed(ActionEvent e) {
        classPane.closeMenus();
        createFileChooser();
        fileChooser.setFileFilter(new FileFilter() {
          public boolean accept(File f) {
            return f.isDirectory() || f.getName().endsWith(".udc");
          }
          public String getDescription() {
            return "*.udc";
          }
        });
        if(fileChooser.showSaveDialog(WorkspaceToolBarMenu.this) == JFileChooser.APPROVE_OPTION) {
          File f = fileChooser.getSelectedFile();
          if(!f.getName().endsWith(".udc")) {
            f = new File(f.getAbsolutePath() + ".udc");
          }
          try {
            Deflater deflater = new Deflater(Deflater.BEST_COMPRESSION);
            OutputStream out = new BufferedOutputStream(new DeflaterOutputStream(new FileOutputStream(f), deflater));
            out.write(Workspace.getXMLDescription(classPane).getBytes("UTF-8"));
            out.flush();
            out.close();
          } catch(Exception ex) {
            ex.printStackTrace();
          }
        }
      }
    });
    workspacePane.add(saveWorkspaceMenuItem);
    ToolBarMenuItem autoLayoutMenuItem = new ToolBarMenuItem("Organize layout");
    autoLayoutMenuItem.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        classPane.closeMenus();
        classPane.adjustLayout();
      }
    });
    workspacePane.add(autoLayoutMenuItem);
    ToolBarMenuItem printPreviewMenuItem = new ToolBarMenuItem("Print with Preview...");
    printPreviewMenuItem.addActionListener(new ActionListener(){
      public void actionPerformed(ActionEvent e) {
        classPane.closeMenus();
        if(classPane.getClassComponentPanePrintBounds() == null) {
          return;
        }
        Window ancestor = SwingUtilities.getWindowAncestor(classPane);
        PrintPreviewDialog dialog;
        if(ancestor instanceof Dialog) {
          dialog = new PrintPreviewDialog((Dialog)ancestor, classPane);
        } else {
          dialog = new PrintPreviewDialog((Frame)ancestor, classPane);
        }
        dialog.pack();
        dialog.setLocationRelativeTo(ancestor);
        dialog.setResizable(false);
        dialog.setModal(true);
        dialog.setVisible(true);
      }
    });
    workspacePane.add(printPreviewMenuItem);
    return workspacePane;
  }

}
