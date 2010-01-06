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
import chrriis.udoc.ui.ClassComponent;
import chrriis.udoc.ui.IconManager;

public class FieldDeclarationComponent extends JPanel {

  protected static final Font FIELD_FONT = new Font("sans-serif", Font.PLAIN, 10);

  public FieldDeclarationComponent(ClassComponent classComponent, ClassInfo classInfo, FieldInfo fieldInfo) {
    super(new FlowLayout(FlowLayout.LEFT, 0, 0));
    setOpaque(false);
    add(new JLabel(IconManager.getIcon(Modifiers.FIELD | fieldInfo.getModifiers())));
    add(new FieldLink(classComponent, fieldInfo.getName(), classInfo, fieldInfo));
    add(new JLabel(" : "));
    add(new ClassDeclarationComponent(classComponent, fieldInfo.getClassDeclaration(), fieldInfo.getClassNamesIndices(), fieldInfo.getClassInfos()));
    for(int i=getComponentCount()-1; i>=0; i--) {
      getComponent(i).setFont(FIELD_FONT);
    }
  }

  public Dimension getMaximumSize() {
    return new Dimension(super.getMaximumSize().width, getPreferredSize().height);
  }

}
