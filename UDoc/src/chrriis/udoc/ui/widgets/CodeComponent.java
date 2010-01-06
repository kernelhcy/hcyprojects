package chrriis.udoc.ui.widgets;

import java.awt.Color;
import java.awt.FlowLayout;
import java.awt.Font;
import java.util.ArrayList;
import java.util.List;

import javax.swing.JLabel;
import javax.swing.JPanel;

import chrriis.udoc.model.Util;

public class CodeComponent extends JPanel {

  protected static final Color IDENTIFIER_COLOR = new Color(127, 0, 85);

  protected static class CodeTokenComponent extends JLabel {
    public CodeTokenComponent(String token) {
      super(token);
      if(Util.isReservedKeyword(token)) {
        setFont(getFont().deriveFont(Font.BOLD, 11));
        setForeground(IDENTIFIER_COLOR);
      } else {
        setFont(getFont().deriveFont(Font.PLAIN, 11));
      }
      setBorder(null);
    }
  }

  public CodeComponent(String code) {
    super(new FlowLayout(FlowLayout.LEFT, 0, 0));
    setOpaque(false);
//    add(new LineComponent(code));
    
    List tokenList = new ArrayList();
    
    for(int i=0; i<code.length(); i++) {
      char c = code.charAt(i);
      switch(c) {
        case ' ':
        case '(':
        case '[':
        case ')':
          if(i > 0) {
            tokenList.add(code.substring(0, i));
          }
          tokenList.add(code.substring(i, i + 1));
          code = code.substring(i + 1);
          i = 0;
          break;
      }
    }
    if(code.length() > 0) {
      tokenList.add(code);
    }
    for(int i=0; i<tokenList.size(); i++) {
      add(new CodeTokenComponent((String)tokenList.get(i)));
    }
    
  }

}
