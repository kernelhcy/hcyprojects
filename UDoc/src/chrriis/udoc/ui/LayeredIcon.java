/*
 * Christopher Deckers (chrriis@nextencia.net)
 * http://www.nextencia.net
 * 
 * See the file "readme.txt" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
package chrriis.udoc.ui;

import java.awt.Component;
import java.awt.Graphics;
import java.awt.Image;
import java.awt.Point;
import java.awt.image.ImageObserver;
import java.util.ArrayList;
import java.util.List;

import javax.swing.Icon;
import javax.swing.ImageIcon;

public class LayeredIcon extends ImageIcon {

  protected List iconList = new ArrayList();
  protected List positionsList = new ArrayList();

  protected int width;
  protected int height;

  public LayeredIcon(int width, int height) {
    this.width = width;
    this.height = height;
  }

  public int getIconWidth() {
    return width;
  }

  
  public int getIconHeight() {
    return height;
  }

//  protected void computeAttributes() {
//    width = 0;
//    height = 0;
//    for(int i=iconList.size()-1; i>=0; i--) {
//      int offsetX = 0;
//      switch(i) {
//        case 1: offsetX += 15; break;
//        case 2: offsetX += 8; break;
//        case 3: offsetX ++; break;
//      }
//      Icon icon = (Icon)iconList.get(i);
//      width = Math.max(width, icon.getIconWidth() + offsetX);
//      height = Math.max(height, icon.getIconHeight());
//    }
//  }

  protected Component component;

  public void paintIcon(Component component, Graphics g, int x, int y) {
    this.component = component;
    for(int i=0; i<iconList.size(); i++) {
      Point position = (Point)positionsList.get(i);
      ((Icon)iconList.get(i)).paintIcon(component, g, x + position.x, y + position.y);
    }
  }

  protected int lastTopPosition;

  public void addTopDecorationIcon(Icon icon) {
    int offsetX = 0;
    switch(lastTopPosition++) {
      case 0: offsetX += 15; break;
      case 1: offsetX += 8; break;
      case 2: offsetX ++; break;
    }
    addIcon(icon, new Point(offsetX, 0));
  }

  protected int lastBottomPosition;

  public void addBottomDecorationIcon(Icon icon) {
    int offsetX = 0;
    switch(lastBottomPosition++) {
      case 0: offsetX += 15; break;
      case 1: offsetX += 8; break;
      case 2: offsetX ++; break;
    }
    addIcon(icon, new Point(offsetX, 7));
  }
  
  public void addIcon(Icon icon) {
    addIcon(icon, new Point(0, 0));
  }

  protected ImageObserver observer;

  protected Image currentImage;

  public void addIcon(Icon icon, Point position) {
    if(icon instanceof ImageIcon) {
      if(observer == null) {
        observer = new ImageObserver() {
          public boolean imageUpdate(Image img, int infoflags, int x, int y, int width, int height) {
            if(component != null) {
              currentImage = img;
              boolean result = component.imageUpdate(img, infoflags, x, y, width, height);
              currentImage = null;
              return result;
            }
            return false;
          }
        };
      }
      ((ImageIcon)icon).setImageObserver(observer);
    }
    iconList.add(icon);
    positionsList.add(position);
//    computeAttributes();
  }

  public Image getImage() {
    return currentImage;
  }

}
