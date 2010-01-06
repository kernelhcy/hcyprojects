/*
 * Christopher Deckers (chrriis@nextencia.net)
 * http://www.nextencia.net
 * 
 * See the file "readme.txt" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
package chrriis.udoc.ui.toolbar;

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Component;
import java.awt.Dimension;

import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTabbedPane;
import javax.swing.JTextArea;

import chrriis.udoc.ui.ClassPane;
import chrriis.udoc.ui.IconManager;

public class HelpToolBarMenu extends ToolBarMenu {

  protected static final Color SELECTION_COLOR = new Color(218, 219, 255);

  protected ClassPane classPane;

  public HelpToolBarMenu(ClassPane classPane) {
    super(IconManager.getIcon("tool_bar_on.gif"), IconManager.getIcon("tool_bar_off.gif"), classPane);
    this.classPane = classPane;
  }

  protected String getContentTitle() {
    return "Help";
  }

  protected Color getSelectionColor() {
    return SELECTION_COLOR;
  }

  protected Component createPopupMenuContent() {
    JPanel helpPane = new JPanel(new BorderLayout(0, 0));
    JTabbedPane tabbedPane = new JTabbedPane(JTabbedPane.TOP, JTabbedPane.SCROLL_TAB_LAYOUT);
    JTextArea generalHelpTextArea = new JTextArea();
    generalHelpTextArea.setText(getGeneralHelpText());
    generalHelpTextArea.setLineWrap(true);
    generalHelpTextArea.setWrapStyleWord(true);
    generalHelpTextArea.setEditable(false);
    generalHelpTextArea.setCaretPosition(0);
    tabbedPane.addTab("General", new JScrollPane(generalHelpTextArea));
    JTextArea controlsHelpTextArea = new JTextArea();
    controlsHelpTextArea.setText(getControlsHelpText());
    controlsHelpTextArea.setLineWrap(true);
    controlsHelpTextArea.setWrapStyleWord(true);
    controlsHelpTextArea.setEditable(false);
    controlsHelpTextArea.setCaretPosition(0);
    tabbedPane.addTab("Controls", new JScrollPane(controlsHelpTextArea));
    helpPane.add(tabbedPane, BorderLayout.CENTER);
    helpPane.setPreferredSize(new Dimension(400, 280));
    return helpPane;
  }

  protected String getGeneralHelpText() {
    return "* Adding a class is achieved from the \"Classes\" menu, using an appropriate class processor.\n\n"
         + "* Classes can be removed only if they form an independant cycle.\n\n"
         + "* Classes can be filtered out using automatic filters (based on their names), or through advanced filters (using wildcards or regular expressions)";
  }

  protected String getControlsHelpText() {
    return "* F5: Load classes. Act on the selected classes, or on all the classes if non are selected.\n\n"
         + "* DEL: Filter the selected classes out, using an automatic name-based filter.\n\n"
         + "* Shift + Del: Remove the selected classes, provided they form an independant cycle.\n\n"
         + "* Click on a link: Show the full prototype, provided the class is loaded.\n\n"
         + "* Double click on a link: may be bound to an action depending on the class processor (like show the Java documentation)\n\n"
         + "* Shift + Mouse Drag: Select classes using a rectangular marquee, by dragging onto the desktop area.\n\n"
         + "* Ctrl + A: Select all the classes that are currently visible.\n\n"
         + "* Ctrl + L: Organize the layout automatically.\n\n"
         + "* Shift + F5: Crawl the classes. Press again to stop crawling.";
  }

}
