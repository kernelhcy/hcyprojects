/*
 * Christopher Deckers (chrriis@nextencia.net)
 * http://www.nextencia.net
 * 
 * See the file "readme.txt" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
package chrriis.udoc.ui.widgets;

import java.awt.Dimension;
import java.awt.FlowLayout;
import java.awt.Font;

import javax.swing.JLabel;
import javax.swing.JPanel;

import chrriis.udoc.model.ClassInfo;
import chrriis.udoc.model.Modifiers;
import chrriis.udoc.model.FieldInfo;
import chrriis.udoc.model.MethodInfo;
import chrriis.udoc.ui.ClassComponent;
import chrriis.udoc.ui.IconManager;

public class MethodDeclarationComponent extends JPanel {
  
  protected static final Font METHOD_FONT = new Font("sans-serif", Font.PLAIN, 10);

  public MethodDeclarationComponent(ClassComponent classComponent, ClassInfo classInfo, MethodInfo method) {
    super(new FlowLayout(FlowLayout.LEFT, 0, 0));
    setOpaque(false);
    int modifiers = method.getReturnedParameter() == null? Modifiers.CONSTRUCTOR: Modifiers.METHOD;
    modifiers |= method.getModifiers();
    if((method.getClassInfo().getModifiers() & Modifiers.ANNOTATION) != 0) {
      modifiers &= ~Modifiers.ABSTRACT;
    }
    add(new JLabel(IconManager.getIcon(modifiers)));
    add(new MethodLink(classComponent, method.getName(), classInfo, method));
    add(new JLabel("("));
    FieldInfo[] fieldInfos = method.getParameters();
    for(int i=0; i<fieldInfos.length; i++) {
      FieldInfo field = fieldInfos[i];
      if(i > 0) {
        add(new JLabel(", "));
      }
      add(new ClassDeclarationComponent(classComponent, field.getClassDeclaration(), field.getClassNamesIndices(), field.getClassInfos()));
    }
    add(new JLabel(")"));
    for(int i=getComponentCount()-1; i>=0; i--) {
      getComponent(i).setFont(METHOD_FONT);
    }
  }

  public Dimension getMaximumSize() {
    return new Dimension(super.getMaximumSize().width, getPreferredSize().height);
  }

}
