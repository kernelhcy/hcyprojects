/*
 * Christopher Deckers (chrriis@nextencia.net)
 * http://www.nextencia.net
 * 
 * See the file "readme.txt" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
package chrriis.udoc.ui.widgets;

import java.awt.AWTEvent;
import java.awt.Color;
import java.awt.Component;
import java.awt.Cursor;
import java.awt.Point;
import java.awt.event.MouseEvent;

import javax.swing.JLabel;
import javax.swing.SwingUtilities;

public abstract class Link extends JLabel {

  protected Component rootParent;

  public Link(Component rootParent, String text) {
    super(text);
    this.rootParent = rootParent;
    enableEvents(AWTEvent.MOUSE_EVENT_MASK | AWTEvent.MOUSE_MOTION_EVENT_MASK);
    setCursor(Cursor.getPredefinedCursor(Cursor.HAND_CURSOR));
  }

  protected Color oldForeground;

  protected void processMouseMotionEvent(MouseEvent e) {
    Point p = getLocation();
    SwingUtilities.convertPoint(this, p, rootParent);
    rootParent.dispatchEvent(e);
    super.processMouseMotionEvent(e);
  }

  protected void processMouseEvent(MouseEvent e) {
    super.processMouseEvent(e);
    switch(e.getID()) {
      case MouseEvent.MOUSE_ENTERED:
        oldForeground = getForeground();
        setForeground(Color.blue);
        break;
      case MouseEvent.MOUSE_EXITED:
        setForeground(oldForeground);
        oldForeground = null;
        break;
    }
    Point p = getLocation();
    SwingUtilities.convertPoint(this, p, rootParent);
    rootParent.dispatchEvent(e);
    switch(e.getID()) {
      case MouseEvent.MOUSE_CLICKED:
        if(e.getClickCount() == 2) {
          processLink();
        }
        break;
    }
  }

  protected abstract void processLink();

}
