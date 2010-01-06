/*
 * Christopher Deckers (chrriis@nextencia.net)
 * http://www.nextencia.net
 * 
 * See the file "readme.txt" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
package chrriis.udoc.ui.toolbar;

import java.awt.Color;
import java.awt.Cursor;
import java.awt.Dimension;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;

import javax.swing.BorderFactory;
import javax.swing.Icon;
import javax.swing.JComponent;
import javax.swing.JLabel;

public class ToolBarMenuItem extends JLabel {

  protected static final Color SELECTION_COLOR = new Color(218, 219, 255);

  protected JComponent menuPane;

  public ToolBarMenuItem(String text) {
    this(text, null);
  }

  public ToolBarMenuItem(String text, Icon icon) {
    super(text, icon, JLabel.LEFT);
    setBorder(BorderFactory.createEmptyBorder(1, 5, 1, 5));
    setBackground(SELECTION_COLOR);
    addMouseListener(new MouseAdapter() {
      public void mouseEntered(MouseEvent e) {
        int modifiers = e.getModifiers();
        if((modifiers & MouseEvent.BUTTON1_MASK) != 0 || (modifiers & MouseEvent.BUTTON2_MASK) != 0 || (modifiers & MouseEvent.BUTTON3_MASK) != 0) {
          return;
        }
        setCursor(Cursor.getPredefinedCursor(Cursor.HAND_CURSOR));
        setOpaque(true);
        repaint();
      }
      public void mouseExited(MouseEvent e) {
        setCursor(null);
        setOpaque(false);
        repaint();
      }
      public void mousePressed(MouseEvent e) {
        if(e.getButton() != MouseEvent.BUTTON1) {
          return;
        }
        fireActionPerformed(new ActionEvent(ToolBarMenuItem.this, ActionEvent.ACTION_PERFORMED, null));
      }
    });
  }
  
  protected void fireActionPerformed(ActionEvent event) {
    Object[] listeners = listenerList.getListenerList();
    for(int i=listeners.length-2; i>=0; i-=2) {
      if(listeners[i] == ActionListener.class) {
        ((ActionListener)listeners[i+1]).actionPerformed(event);
      }          
    }
  }
  
  public void addActionListener(ActionListener l) {
    listenerList.add(ActionListener.class, l);
  }
  
  public void removeActionListener(ActionListener l) {
    listenerList.remove(ActionListener.class, l);
  }
  
  public Dimension getMaximumSize() {
    return new Dimension(Integer.MAX_VALUE, super.getMaximumSize().height);
  }
  
}
