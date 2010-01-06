/*
 * Christopher Deckers (chrriis@nextencia.net)
 * http://www.nextencia.net
 * 
 * See the file "readme.txt" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
package chrriis.udoc.ui.webpage;

import java.awt.BorderLayout;
import java.awt.Rectangle;

import javax.swing.JEditorPane;
import javax.swing.JFrame;
import javax.swing.JScrollPane;
import javax.swing.SwingUtilities;
import javax.swing.event.HyperlinkEvent;
import javax.swing.event.HyperlinkListener;

public class WebpageViewer extends JFrame {

  protected JEditorPane editorPane;

  protected WebpageViewer() {
    setLocationByPlatform(true);
    editorPane = new JEditorPane();
    editorPane.addHyperlinkListener(new HyperlinkListener() {
      public void hyperlinkUpdate(HyperlinkEvent e) {
        if(e.getEventType() != HyperlinkEvent.EventType.ACTIVATED) {
          return;
        }
        try {
          openPage(e.getURL().toString());
        } catch(Exception ex) {}
      }
    });
    editorPane.setEditable(false);
    getContentPane().add(new JScrollPane(editorPane), BorderLayout.CENTER);
    setSize(800, 600);
  }

  protected static WebpageViewer instance;

  public static void open(String title, String documentPath) {
    if(instance == null) {
      instance = new WebpageViewer();
    }
    instance.openPage(title, documentPath);
    instance.setVisible(true);
    SwingUtilities.invokeLater(new Runnable() {
      public void run() {
        instance.requestFocus();
        instance.toFront();
      }
    });
  }

  public void openPage(String title, final String documentPath) {
    setTitle(title);
    openPage(documentPath);
  }

  public void openPage(final String documentPath) {
    try {
      final int index = documentPath.indexOf('#');
      String page = index < 0? documentPath: documentPath.substring(0, index);
      editorPane.setPage(page);
      new Thread() {
        public void run() {
          long now = System.currentTimeMillis();
          while(System.currentTimeMillis() - now < 5000 && editorPane.getY() < 0) {
            SwingUtilities.invokeLater(new Runnable() {
              public void run() {
                editorPane.scrollRectToVisible(new Rectangle(0, 0, editorPane.getWidth(), 1));
              }
            });
            try {
              sleep(400);
            } catch(Exception e) {}
          }
          if(index > 0) {
            now = System.currentTimeMillis();
            while(System.currentTimeMillis() - now < 5000 && editorPane.getY() >= 0) {
              SwingUtilities.invokeLater(new Runnable() {
                public void run() {
                  editorPane.scrollToReference(documentPath.substring(index + 1));
                }
              });
              try {
                sleep(400);
              } catch(Exception e) {}
            }
          }
        }
      }.start();
    } catch(Exception e) {
      editorPane.setText("Could not open " + documentPath);
    }
    toFront();
  }

}
