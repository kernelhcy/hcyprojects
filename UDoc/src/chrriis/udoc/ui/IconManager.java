/*
 * Christopher Deckers (chrriis@nextencia.net)
 * http://www.nextencia.net
 * 
 * See the file "readme.txt" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
package chrriis.udoc.ui;

import java.awt.Image;
import java.awt.Point;
import java.awt.Toolkit;
import java.util.HashMap;

import javax.swing.Icon;
import javax.swing.ImageIcon;

import chrriis.udoc.model.Modifiers;

public class IconManager implements Modifiers {

  protected static HashMap modifierToIconMap = new HashMap();
  protected static HashMap iconNameToImageMap = new HashMap();

  protected static final Point STATUS_POSITION = new Point(7, 7);

  public static Icon getIcon(int modifiers) {
    LayeredIcon icon = (LayeredIcon)modifierToIconMap.get(new Integer(modifiers));
    if(icon != null) {
      return icon;
    }
    icon = new LayeredIcon(24, 16);
    if((modifiers & DEPRECATED) != 0) {
      icon.addIcon(getIcon("deprecated.gif"));
    }
    if((modifiers & NOT_LOADED) != 0 || (modifiers & LOADING) != 0 || (modifiers & LOADING_FAILED) != 0) {
      if((modifiers & CLASS) != 0) {
        icon.addIcon(getIcon("class_status.gif"));
      } else if((modifiers & INTERFACE) != 0) {
        icon.addIcon(getIcon("interface_status.gif"));
      } else {
        icon.addIcon(getIcon("unknown_type_status.gif"));
      }
      if((modifiers & LOADING_FAILED) != 0) {
        icon.addIcon(getIcon("loading_failed.gif"), STATUS_POSITION);
      } else if((modifiers & NOT_LOADED) != 0) {
          icon.addIcon(getIcon("not_loaded.gif"), STATUS_POSITION);
      } else {
        icon.addIcon(getIcon("loading.gif"), STATUS_POSITION);
      }
    } else {
      if((modifiers & FIELD) != 0) {
        if((modifiers & PUBLIC) != 0) {
          icon.addIcon(getIcon("field_public.gif"));
        } else if((modifiers & PROTECTED) != 0) {
          icon.addIcon(getIcon("field_protected.gif"));
        } else if((modifiers & PRIVATE) != 0) {
          icon.addIcon(getIcon("field_private.gif"));
        } else {
          icon.addIcon(getIcon("field_default.gif"));
        }
      } else if((modifiers & METHOD) != 0 || (modifiers & CONSTRUCTOR) != 0) {
        if((modifiers & PUBLIC) != 0) {
          icon.addIcon(getIcon("method_public.gif"));
        } else if((modifiers & PROTECTED) != 0) {
          icon.addIcon(getIcon("method_protected.gif"));
        } else if((modifiers & PRIVATE) != 0) {
          icon.addIcon(getIcon("method_private.gif"));
        } else {
          icon.addIcon(getIcon("method_default.gif"));
        }
        if((modifiers & CONSTRUCTOR) != 0) {
          icon.addTopDecorationIcon(getIcon("constructor_co.gif"));
        }
        if((modifiers & ABSTRACT) != 0) {
          icon.addTopDecorationIcon(getIcon("abstract_co.gif"));
        }
      } else {
        // Types
        if((modifiers & PUBLIC) != 0) {
          if((modifiers & ANNOTATION) != 0) {
            icon.addIcon(getIcon("annotation_public.gif"));
          } else if((modifiers & ENUM) != 0) {
            icon.addIcon(getIcon("enum_public.gif"));
          } else if((modifiers & INTERFACE) != 0) {
            icon.addIcon(getIcon("interface_public.gif"));
          } else if((modifiers & CLASS) != 0) {
            icon.addIcon(getIcon("class_public.gif"));
          }
        } else {
          if((modifiers & ANNOTATION) != 0) {
            icon.addIcon(getIcon("annotation_status.gif"));
          } else if((modifiers & ENUM) != 0) {
            icon.addIcon(getIcon("enum_status.gif"));
          } else if((modifiers & INTERFACE) != 0) {
            icon.addIcon(getIcon("interface_status.gif"));
          } else if((modifiers & CLASS) != 0) {
            icon.addIcon(getIcon("class_status.gif"));
          }
          if((modifiers & PROTECTED) != 0) {
            icon.addIcon(getIcon("type_protected.gif"), STATUS_POSITION);
          } else if((modifiers & PRIVATE) != 0) {
            icon.addIcon(getIcon("type_private.gif"), STATUS_POSITION);
          } else {
            icon.addIcon(getIcon("type_default.gif"), STATUS_POSITION);
          }
        }
        if((modifiers & ABSTRACT) != 0) {
          icon.addTopDecorationIcon(getIcon("abstract_co.gif"));
        }
      }
      if((modifiers & FINAL) != 0) {
        icon.addTopDecorationIcon(getIcon("final_co.gif"));
      }
      if((modifiers & STATIC) != 0) {
        icon.addTopDecorationIcon(getIcon("static_co.gif"));
      }
      if((modifiers & VOLATILE) != 0) {
        icon.addTopDecorationIcon(getIcon("volatile_co.gif"));
      }
      if((modifiers & NATIVE) != 0) {
        icon.addBottomDecorationIcon(getIcon("native_co.gif"));
      }
      if((modifiers & SYNCHRONIZED) != 0) {
        icon.addBottomDecorationIcon(getIcon("synchronized_co.gif"));
      }
    }
    modifierToIconMap.put(new Integer(modifiers), icon);
    return icon;
  }

  public static Image getImage(String name) {
    Image image = (Image)iconNameToImageMap.get(name);
    if(image != null) {
      return image;
    }
    image = Toolkit.getDefaultToolkit().getImage(IconManager.class.getResource("resources/" + name));
    iconNameToImageMap.put(name, image);
    return image;
  }

  public static Icon getIcon(String name) {
    return new ImageIcon(getImage(name));
  }

}
