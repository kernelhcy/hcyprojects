/*
 * Christopher Deckers (chrriis@nextencia.net)
 * http://www.nextencia.net
 * 
 * See the file "readme.txt" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
package chrriis.udoc.model;

public class Util {

  public static String escapeClassName(String className) {
    int length = className.length();
    StringBuffer sb = new StringBuffer(length);
    for(int i=0; i<length; i++) {
      char c = className.charAt(i);
      switch(c) {
        case '/': sb.append('.'); break;
        case '.': sb.append('$'); break;
        case '$': sb.append("$$"); break;
        default: sb.append(c); break;
      }
    }
    return sb.toString();
  }

  public static String unescapeClassName(String className) {
    int length = className.length();
    StringBuffer sb = new StringBuffer(length);
    for(int i=0; i<length; i++) {
      char c = className.charAt(i);
      switch(c) {
        case '$':
          if(i < length-1) {
            char c2 = className.charAt(i + 1);
            if(c2 != '$') {
              sb.append('.');
            }
            sb.append(c2);
            i++;
          } else {
            sb.append('.');
          }
          break;
        default: sb.append(c); break;
      }
    }
    return sb.toString();
  }

  public static int getModifiers(String prototype) {
    int modifiers = 0;
    for(int index; (index = prototype.indexOf(' ')) > 0; ) {
      String token = prototype.substring(0, index);
      int modifier = getModifier(token);
      if(modifier < 0) {
        break;
      }
      modifiers |= modifier;
      prototype = prototype.substring(index + 1);
    }
    return modifiers;
  }

  protected static String getModifiers(int modifiers) {
    StringBuffer sb = new StringBuffer();
    if((modifiers & Modifiers.PUBLIC) != 0) {
      sb.append("public ");
    } else if((modifiers & Modifiers.PROTECTED) != 0) {
      sb.append("protected ");
    } else if((modifiers & Modifiers.PRIVATE) != 0) {
      sb.append("private ");
    }
    if((modifiers & Modifiers.ABSTRACT) != 0 && (modifiers & (Modifiers.INTERFACE | Modifiers.ANNOTATION | Modifiers.ENUM)) == 0) {
      sb.append("abstract ");
    }
    if((modifiers & Modifiers.STATIC) != 0) {
      sb.append("static ");
    }
    if((modifiers & Modifiers.FINAL) != 0) {
      sb.append("final ");
    }
    if((modifiers & Modifiers.NATIVE) != 0) {
      sb.append("native");
    }
    if((modifiers & Modifiers.VOLATILE) != 0) {
      sb.append("volatile");
    }
    if((modifiers & Modifiers.TRANSIENT) != 0) {
      sb.append("transient");
    }
    if((modifiers & Modifiers.SYNCHRONIZED) != 0) {
      sb.append("synchronized");
    }
    if((modifiers & Modifiers.ENUM) != 0) {
      sb.append("enum ");
    } else if((modifiers & Modifiers.ANNOTATION) != 0) {
      sb.append("@interface ");
    } else if((modifiers & Modifiers.INTERFACE) != 0) {
      sb.append("interface ");
    } else if((modifiers & Modifiers.CLASS) != 0) {
      sb.append("class ");
    }
    return sb.toString();
  }

  public static boolean isModifier(String modifier) {
    return getModifier(modifier) != -1;
  }
  
  public static int getModifier(String modifier) {
    if("public".equals(modifier)) {
      return Modifiers.PUBLIC;
    }
    if("protected".equals(modifier)) {
      return Modifiers.PROTECTED;
    }
    if("private".equals(modifier)) {
      return Modifiers.PRIVATE;
    }
    if("@interface".equals(modifier)) {
      return Modifiers.ANNOTATION;
    }
    if("enum".equals(modifier)) {
      return Modifiers.ENUM;
    }
    if("interface".equals(modifier)) {
      return Modifiers.INTERFACE;
    }
    if("abstract".equals(modifier)) {
      return Modifiers.ABSTRACT;
    }
    if("class".equals(modifier)) {
      return Modifiers.CLASS;
    }
    if("static".equals(modifier)) {
      return Modifiers.STATIC;
    }
    if("final".equals(modifier)) {
      return Modifiers.FINAL;
    }
    if("native".equals(modifier)) {
      return Modifiers.NATIVE;
    }
    if("volatile".equals(modifier)) {
      return Modifiers.VOLATILE;
    }
    if("transient".equals(modifier)) {
      return Modifiers.TRANSIENT;
    }
    if("synchronized".equals(modifier)) {
      return Modifiers.SYNCHRONIZED;
    }
    return -1;
  }

  public static String escapeXML(String s) {
    if(s == null || s.length() == 0) {
      return s;
    }
    StringBuffer sb = new StringBuffer((int)(s.length() * 1.1));
    for(int i=0; i<s.length(); i++) {
      char c = s.charAt(i);
      switch(c) {
        case '<':
          sb.append("&lt;");
          break;
        case '>':
          sb.append("&gt;");
          break;
        case '&':
          sb.append("&amp;");
          break;
        case '\'':
          sb.append("&apos;");
          break;
        case '\"':
          sb.append("&quot;");
          break;
        default:
          sb.append(c);
        break;
      }
    }
    return sb.toString();
  }

  public static boolean isReservedKeyword(String word) {
    if(Util.getModifier(word) != -1) {
      return true;
    }
    if("void".equals(word) ||
        "boolean".equals(word) ||
        "byte".equals(word) ||
        "char".equals(word) ||
        "short".equals(word) ||
        "int".equals(word) ||
        "long".equals(word) ||
        "float".equals(word) ||
        "double".equals(word) ||
        "throws".equals(word) ||
        "implements".equals(word) ||
        "extends".equals(word)) {
      return true;
    }
    return false;
  }

  public static int getClassModifiers(String prototype) {
    while(prototype.startsWith("@") && !prototype.startsWith("@interface ")) {
      int count = 0;
      for(int i=0; i<prototype.length(); i++) {
        char c = prototype.charAt(i);
        switch(c) {
          case '(': count++; break;
          case ')': count--; break;
          case ' ':
            if(count == 0) {
              prototype = prototype.substring(i + 1);
              i = prototype.length();
            }
            break;
        }
      }
    }
    String[] tokens = prototype.split(" ");
    int classModifiers = 0;
    for(int i=0; i<tokens.length; i++) {
      int modifier = Util.getModifier(tokens[i]);
      if(modifier == -1) {
        break;
      }
      classModifiers |= modifier;
    }
    return classModifiers;
  }

  protected static String pathSepartor;
  
  public static String getPathSeparator() {
    if(pathSepartor != null) {
      return pathSepartor;
    }
    return System.getProperty("path.separator");
  }
  
  public static void setPathSeparator(String pathSepartor) {
    Util.pathSepartor = pathSepartor;
  }
  
  protected static boolean isRestricted;
  
  public static boolean isRestricted() {
    return isRestricted;
  }
  
  public static void setRestricted(boolean isRestricted) {
    Util.isRestricted = isRestricted;
  }
  
}
