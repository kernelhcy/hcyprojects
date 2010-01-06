package chrriis.udoc.ui.widgets;

import java.awt.Color;
import java.util.ArrayList;
import java.util.List;

import javax.swing.BorderFactory;
import javax.swing.BoxLayout;
import javax.swing.JPanel;
import javax.swing.border.Border;

import chrriis.udoc.model.ClassInfo;
import chrriis.udoc.model.Util;

public class ClassTooltip extends JPanel {

  protected static final Color TOOLTIP_COLOR = new Color(255, 255, 160);
  protected static final Border BORDER = BorderFactory.createCompoundBorder(BorderFactory.createLineBorder(new Color(167, 166, 174)), BorderFactory.createEmptyBorder(1, 1, 1, 1));
  protected static final Color BACKGROUND_COLOR = new Color(255, 255, 160);

  public ClassTooltip(ClassInfo classInfo) {
    BoxLayout boxLayout = new BoxLayout(this, BoxLayout.Y_AXIS);
    setLayout(boxLayout);
    setOpaque(true);
    setBorder(BORDER);
    setBackground(BACKGROUND_COLOR);
    String prototype = classInfo.getPrototype();
    List tokenList = new ArrayList();
    boolean isValid = true;
    while(isValid && prototype.startsWith("@")) {
      for(int i=0; isValid && i<prototype.length(); i++) {
        int count = 0;
        char c = prototype.charAt(i);
        switch(c) {
          case '(': count++; break;
          case ')': count--; break;
          case ' ':
            if(count == 0) {
              String s = prototype.substring(0, i);
              if(Util.getModifier(s) == -1) {
                tokenList.add(s);
                prototype = prototype.substring(i + 1);
                i = 0;
              } else {
                isValid = false;
              }
            }
            break;
        }
      }
    }
    int index = prototype.indexOf(" extends ");
    if(index != -1) {
      tokenList.add(prototype.substring(0, index).trim());
      prototype = prototype.substring(index + 1).trim();
    }
    index = prototype.indexOf(" implements ");
    if(index != -1) {
      tokenList.add(prototype.substring(0, index).trim());
      prototype = prototype.substring(index + 1).trim();
    }
    tokenList.add(prototype);
    for(int i=0; i<tokenList.size(); i++) {
      add(new CodeComponent((String)tokenList.get(i)));
    }
  }

}
