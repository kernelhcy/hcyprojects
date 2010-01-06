/*
 * Christopher Deckers (chrriis@nextencia.net)
 * http://www.nextencia.net
 * 
 * See the file "readme.txt" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
package chrriis.udoc.ui;

import java.util.regex.Pattern;

import chrriis.udoc.model.ClassInfo;

public class Filter {

  protected String namePattern;
  protected boolean isRegularExpression;
  protected Pattern pattern;
  protected int modifiers;
  protected int hashCode;

  public Filter(String namePattern) {
    this(namePattern, false, 0);
  }

  public Filter(String namePattern, boolean isRegularExpression, int modifiers) {
    if(isRegularExpression) {
      pattern = Pattern.compile(namePattern);
      hashCode = namePattern.hashCode() + modifiers;
    } else {
      String regExp = "\\Q" + namePattern.replace("\\E", "\\\\E").replace("\\Q", "\\\\Q").replace("?", "\\E.\\Q").replace("*", "\\E.*\\Q");
      hashCode = regExp.hashCode() + modifiers;
      pattern = Pattern.compile(regExp);
    }
    this.namePattern = namePattern;
    this.isRegularExpression = isRegularExpression;
    this.modifiers = modifiers;
  }

  public boolean matches(ClassInfo classInfo) {
    return (modifiers == 0 || ((classInfo.getModifiers() & modifiers) != 0)) && pattern.matcher(classInfo.getClassName()).matches();
  }

  public int hashCode() {
    return hashCode;
  }

  public String getNamePattern() {
    return namePattern;
  }

  public boolean isRegularExpression() {
    return isRegularExpression;
  }

  public int getModifiers() {
    return modifiers;
  }

}