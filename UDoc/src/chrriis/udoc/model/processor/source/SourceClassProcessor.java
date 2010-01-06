package chrriis.udoc.model.processor.source;

import java.io.BufferedInputStream;
import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FilenameFilter;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.Reader;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.StringTokenizer;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

import javax.swing.JComponent;
import javax.swing.JTree;
import javax.swing.SwingUtilities;
import javax.swing.tree.DefaultMutableTreeNode;
import javax.swing.tree.DefaultTreeModel;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;

import org.w3c.dom.Document;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

import chrriis.udoc.model.Modifiers;
import chrriis.udoc.model.Util;
import chrriis.udoc.model.processor.ClassProcessor;

public class SourceClassProcessor extends ClassProcessor {

  public String getProcessorID() {
    return "SourceClassProcessor";
  }

  public String getProcessorName() {
    return "Source [EXPERIMENTAL]";
  }
  
  public String getProcessorDescription() {
    return "Load class diagrams from \".java\" source files";
  }
  
  public JComponent getParametersComponent() {
    return new SourceParametersComponent(this);
  }

  protected String sourcePath;
  protected String classPath;

  protected InputStream getSourceInputStream(String className) {
    int lastDotIndex = className.lastIndexOf('.');
    String cName = className.substring(lastDotIndex + 1);
    cName = Util.unescapeClassName(cName);
    int index = cName.indexOf('.');
    if(index != -1) {
      cName = cName.substring(0, index);
    }
    String subPath = (lastDotIndex == -1? cName: className.substring(0, lastDotIndex + 1).replace('.', '/') + cName) + ".java";
    String[] paths = getSourcePath().split(Util.getPathSeparator());
    for(int i=0; i<paths.length; i++) {
      String path = paths[i];
      File file = new File(path);
      if(file.exists()) {
        if(file.isFile()) {
          try {
            ZipInputStream zin = new ZipInputStream(new BufferedInputStream(new FileInputStream(file)));
            for(ZipEntry entry; (entry = zin.getNextEntry()) != null; ) {
              if(subPath.equals(entry.getName())) {
                return zin;
              }
            }
          } catch(Exception e) {
            e.printStackTrace();
          }
        } else {
          try {
            return new BufferedInputStream(new FileInputStream(path + "/" + subPath));
          } catch(Exception e) {
//            e.printStackTrace();
          }
        }
      }
    }
    return getClass().getResourceAsStream("/" + subPath);
  }
  
  protected static final int NONE = 0;
  protected static final int BLOCK_COMMENT = 1;
  protected static final int LINE_COMMENT = 2;
  protected static final int STRING_CONSTANT = 3;
  
  public InputStream getClassInfoDataInputStream(String className) {
    int lastDotIndex = className.lastIndexOf('.');
    String cName = Util.unescapeClassName(className.substring(lastDotIndex + 1));
    String mainName = className.substring(lastDotIndex + 1);
    mainName = Util.unescapeClassName(mainName);
    int dotIndex = mainName.indexOf('.');
    if(dotIndex != -1) {
      mainName = mainName.substring(0, dotIndex);
    }
    String packageName = lastDotIndex == -1? "": className.substring(0, lastDotIndex);
    InputStream in = getSourceInputStream(className);
    if(in == null) {
      return null;
    }
    try {
      Reader reader = new InputStreamReader(in, "UTF-8");
      int backslashCount = 0;
      int level = 0;
      int state = NONE;
      char lastChar = '\0';
      boolean isClassFound = false;
      boolean isInClass = true;
      StringBuffer classNameSB = null;
      String prototype = null;
      String packageDeclaration = "";
      List importList = new ArrayList();
      importList.add(Util.unescapeClassName(className));
      List fieldList = new ArrayList();
      List constructorList = new ArrayList();
      List methodList = new ArrayList();
      StringBuffer tokenSB = new StringBuffer();
      for(int i; level >= 0 && (i = reader.read()) != -1; ) {
        char c = (char)i;
        switch(state) {
          case NONE:
            switch(c) {
              case '\r':
              case '\n':
              case '\t':
                c = ' ';
              case ' ':
                if(level == 0 && lastChar != ' ' && tokenSB.length() > 0) {
                  tokenSB.append(c);
                }
                lastChar = c;
                break;
              case ',':
                if(level == 0) {
                  if(tokenSB.indexOf("<") == -1 && tokenSB.indexOf("(") == -1) {
                    // it probably is an enum value
                  }
                  tokenSB.append(c);
                }
                lastChar = c;
                break;
              case '/':
                if(lastChar == '/') {
                  state = LINE_COMMENT;
                  if(level == 0) {
                    tokenSB.deleteCharAt(tokenSB.length() - 1);
                    lastChar = tokenSB.length() > 0? lastChar = tokenSB.charAt(tokenSB.length() - 1): '\0';
                  } else {
                    lastChar = '\0';
                  }
                } else {
                  if(level == 0) {
                    tokenSB.append(c);
                  }
                  lastChar = c;
                }
                break;
              case '*':
                if(lastChar == '/') {
                  state = BLOCK_COMMENT;
                  if(level == 0) {
                    tokenSB.deleteCharAt(tokenSB.length() - 1);
                    lastChar = tokenSB.length() > 0? lastChar = tokenSB.charAt(tokenSB.length() - 1): '\0';
                  } else {
                    lastChar = '\0';
                  }
                } else {
                  if(level == 0) {
                    tokenSB.append(c);
                  }
                  lastChar = c;
                }
                break;
              case '\"':
                state = STRING_CONSTANT;
                backslashCount = 0;
                lastChar = c;
                break;
              case ';':
                if(level == 0 && tokenSB.length() > 0) {
                  String s = tokenSB.toString();
                  if(s.startsWith("package ")) {
                    packageDeclaration = s.substring("package ".length()).trim();
                    tokenSB = new StringBuffer();
                  } else if(s.startsWith("import ")) {
                    importList.add(s.substring("import ".length()).trim());
                    tokenSB = new StringBuffer();
                  } else if(isClassFound) {
                    String token = tokenSB.toString();
                    int index = token.indexOf('=');
                    if(index != -1) {
                      token = token.substring(0, index);
                    }
                    token = token.trim();
                    if(token.indexOf('(') != -1) {
                      if(isConstructor(token)) {
                        constructorList.add(token);
                      } else {
                        methodList.add(token);
                      }
                    } else {
                      fieldList.add(token);
                    }
                    tokenSB = new StringBuffer();
                  }
                }
                lastChar = c;
                break;
              case '{':
                if(isInClass) {
                  String s = ' ' + tokenSB.toString();
                  int index = s.indexOf(" class ");
                  if(index != -1) {
                    index += " class ".length();
                  } else {
                    index = s.indexOf(" interface ");
                    if(index != -1) {
                      index += " interface ".length();
                    } else {
                      index = s.indexOf(" enum ");
                      if(index != -1) {
                        index += " enum ".length();
                      } else {
                        index = s.indexOf(" @interface ");
                        if(index != -1) {
                          index += " @interface ".length();
                        }
                      }
                    }
                  }
                  if(index != -1) {
                    if(!isClassFound) {
                      s = s.substring(index).trim();
                      for(int j=0; j<s.length(); j++) {
                        switch(s.charAt(j)) {
                          case ' ':
                          case '<':
                            s = s.substring(0, j);
                            break;
                        }
                      }
                      boolean isValid = false;
                      if(classNameSB == null) {
                        if(s.equals(mainName)) {
                          classNameSB = new StringBuffer();
                          classNameSB.append(packageName + "." + s);
                          isValid = true;
                          isClassFound = true;
                        }
                      } else {
                        String currentClass = classNameSB.toString() + '$' + Util.escapeClassName(s);
                        if(className.equals(currentClass) || className.startsWith(currentClass + '$')) {
                          classNameSB.append('$').append(Util.escapeClassName(s));
                          isValid = true;
                          isClassFound = className.equals(currentClass);
                        }
                      }
                      if(isValid) {
                        level = -1;
                      }
                      if(isClassFound) {
                        prototype = tokenSB.toString().trim();
                      }
                    }
                    tokenSB = new StringBuffer();
                  }
                }
                if(level == 0 && isClassFound) {
                  String s = tokenSB.toString();
                  // This is the case of an anonymous constructor initializer.
                  if(s.trim().length() > 0) {
                    int eIndex = s.indexOf('=');
                    if(eIndex != -1) {
                      fieldList.add(s.substring(0, eIndex).trim());
                    } else {
                      s = s.trim();
                      // This is the case of a static initializer.
                      if(!"static".equals(s)) {
                        if(s.indexOf('(') != -1) {
                          if(isConstructor(s)) {
                            constructorList.add(s);
                          } else {
                            methodList.add(s);
                          }
                        } else {
                          fieldList.add(s);
                        }
                      }
                    }
                  }
                  tokenSB = new StringBuffer();
                  isInClass = false;
                }
                level++;
                lastChar = c;
                break;
              case '}':
                level--;
                if(level == 0) {
                  isInClass = true;
                }
//                if(level == 0) {
//                  tokenSB.append(c);
//                }
                lastChar = c;
                break;
              default:
                if(level == 0) {
                  tokenSB.append(c);
                }
                lastChar = c;
                break;
            }
            break;
          case LINE_COMMENT:
            switch(c) {
              case '\r':
              case '\n':
                state = NONE;
                lastChar = tokenSB.length() > 0? lastChar = tokenSB.charAt(tokenSB.length() - 1): '\0';
                break;
              default:
                lastChar = c;
                break;
            }
            break;
          case BLOCK_COMMENT:
            switch(c) {
              case '/':
                if(lastChar == '*') {
                  state = NONE;
                  lastChar = tokenSB.length() > 0? lastChar = tokenSB.charAt(tokenSB.length() - 1): '\0';
                } else {
                  lastChar = c;
                }
                break;
              default:
                lastChar = c;
                break;
            }
            break;
          case STRING_CONSTANT:
            switch(c) {
              case '\\':
                backslashCount = (backslashCount + 1) % 2;
                if(level == 0) {
                  tokenSB.append(c);
                }
                break;
              case '"':
                if(backslashCount == 0) {
                  state = NONE;
                }
                if(level == 0) {
                  tokenSB.append(c);
                }
                break;
            }
            lastChar = c;
            break;
        }
      }
      if(prototype == null) {
        return null;
      }
      if(packageDeclaration == null) {
        packageDeclaration = "";
      }
      if(!packageName.equals(packageDeclaration)) {
        return null;
      }
//      System.err.println("Package: " + packageDeclaration);
//      System.err.println("Import: " + importList.size());
//      System.err.println("Prototype: " + prototype);
//      System.err.println("--- fields ---");
//      for(int i=0; i<fieldList.size(); i++) {
//        System.err.println(fieldList.get(i));
//      }
//      System.err.println("--- methods ---");
//      for(int i=0; i<methodList.size(); i++) {
//        System.err.println(methodList.get(i));
//      }
//      System.err.println("----------");
//      System.err.print(tokenSB);
      importList.add(packageName + ".*");
      importList.add("java.lang.*");
      String[] imports = resolveImports(importList);
      StringBuffer sourceSB = new StringBuffer();
      sourceSB.append("<type name=\"").append(Util.escapeXML(className)).append("\">");
      prototype = adjustPrototype(prototype, imports);
      sourceSB.append("<prototype value=\"").append(Util.escapeXML(prototype)).append("\"/>");
      
      // find super types from prototype
      int extendsIndex = -1;
      int implementsIndex = -1;
      for(int i=0; i<prototype.length(); i++) {
        char c = prototype.charAt(i);
        int count = 0;
        switch(c) {
          case '<': count++; break;
          case '>': count--; break;
          default:
            if(count == 0) {
              String s = prototype.substring(i);
              if(s.startsWith(" extends ")) {
                extendsIndex = i + " extends ".length();
              } else if(s.startsWith(" implements ")) {
                implementsIndex = i + " implements ".length();
              }
            }
            break;
        }
      }
      int classModifiers = Util.getClassModifiers(prototype);
      StringBuffer superClassesSB = new StringBuffer();
      StringBuffer superInterfacesSB = new StringBuffer();
      if(extendsIndex == -1 && (classModifiers & Modifiers.CLASS) != 0 && !"java.lang.Object".equals(className)) {
        superClassesSB.append("java.lang.Object");
      }
      if(extendsIndex != -1) {
        if(implementsIndex == -1) {
          if((classModifiers & Modifiers.INTERFACE) != 0) {
            superInterfacesSB.append(prototype.substring(extendsIndex));
          } else {
            superClassesSB.append(prototype.substring(extendsIndex));
          }
        } else {
          superClassesSB.append(prototype.substring(extendsIndex, implementsIndex - " implements ".length()));
        }
      }
      if(implementsIndex != -1) {
        superInterfacesSB.append(prototype.substring(implementsIndex));
      }
      sourceSB.append("<superTypes>");
      for(int i=0; i<2; i++) {
        StringBuffer superTypeSB;
        String prefix;
        if(i == 0) {
          superTypeSB = superClassesSB;
          prefix = "class ";
        } else {
          superTypeSB = superInterfacesSB;
          prefix = "interface ";
        }
        String superTypes = superTypeSB.toString();
        if(superTypes.length() > 0) {
          int count = 0;
          for(int j=0; j<superTypes.length(); j++) {
            char c = superTypes.charAt(j);
            switch(c) {
              case '<': count++; break;
              case '>': count--; break;
              case ',':
                if(count == 0) {
                  sourceSB.append("<superType value=\"").append(Util.escapeXML(prefix + superTypes.substring(0, j).trim())).append("\"/>");
                  superTypes = superTypes.substring(j + 1);
                  j = 0;
                }
                break;
            }
          }
          sourceSB.append("<superType value=\"").append(Util.escapeXML(prefix + superTypes.trim())).append("\"/>");
        }
      }
      sourceSB.append("</superTypes>");
      //TODO: separate constructors and enums from methods
      sourceSB.append("<constructors>");
      if(constructorList.isEmpty()) {
        sourceSB.append("<constructor value=\"public ").append(Util.escapeXML(cName)).append("()\"/>");
      } else {
        for(int i=0; i<constructorList.size(); i++) {
          sourceSB.append("<constructor value=\"").append(Util.escapeXML(adjustMethod((String)constructorList.get(i), imports))).append("\"/>");
        }
      }
      sourceSB.append("</constructors>");
      String methodType;
      if((classModifiers & Modifiers.ANNOTATION) != 0) {
        methodType = "annotationMember";
      } else {
        methodType = "method";
      }
      sourceSB.append('<').append(methodType).append("s>");
      for(int i=0; i<methodList.size(); i++) {
        sourceSB.append('<').append(methodType).append(" value=\"").append(Util.escapeXML(adjustMethod((String)methodList.get(i), imports))).append("\"/>");
      }
      sourceSB.append("</").append(methodType).append("s>");
      //TODO: separate fields, annotation fields, etc.
      sourceSB.append("<fields>");
      for(int i=0; i<fieldList.size(); i++) {
        sourceSB.append("<field value=\"").append(Util.escapeXML(adjustField((String)fieldList.get(i), imports))).append("\"/>");
      }
      sourceSB.append("</fields>");
      sourceSB.append("</type>");
      System.err.println(sourceSB);
      return new ByteArrayInputStream(sourceSB.toString().getBytes("UTF-8"));
    } catch(Exception e) {
      e.printStackTrace();
    }
    return null;
  }

  protected static boolean isConstructor(String token) {
    while(token.startsWith("@") && !token.startsWith("@interface ")) {
      int count = 0;
      for(int i=0; i<token.length(); i++) {
        char c = token.charAt(i);
        switch(c) {
          case '(': count++; break;
          case ')': count--; break;
          case ' ':
            if(count == 0) {
              token = token.substring(i + 1);
              i = token.length();
            }
            break;
        }
      }
    }
    String[] tokens = token.split(" ");
    boolean isConstructor = false;
    for(int i=0; i<tokens.length; i++) {
      String t = tokens[i];
      if(!Util.isModifier(t)) {
        
        isConstructor = t.indexOf('(') != -1;
        break;
      }
    }
    return isConstructor;
  }

  protected static String adjustPrototype(String prototype, String[] imports) {
    prototype = prototype.replaceAll(" <", "<")
                         .replaceAll("< ", "<")
                         .replaceAll(" >", ">")
                         .replaceAll(" \\?", "\\?")
                         .replaceAll(", ", ",")
                         .replaceAll(",", ", ");
    StringBuffer sb = new StringBuffer();
    StringTokenizer st = new StringTokenizer(prototype, ", <>", true);
    while(st.hasMoreTokens()) {
      String token = st.nextToken();
      if(token.length() > 0) {
        boolean isAdded = false;
        if(token.length() == 1) {
          switch(token.charAt(0)) {
            case ',':
            case ' ':
            case '<':
            case '>':
              isAdded = true;
              sb.append(token);
              break;
          }
        }
        if(!isAdded) {
          if(Util.isReservedKeyword(token)) {
            sb.append(token);
          } else {
            sb.append(getFullyQualifiedName(token, imports));
          }
        }
      }
    }
    return sb.toString();
  }

  protected static String adjustMethod(String prototype, String[] imports) {
    prototype = prototype.replaceAll(" \\[", "\\[")
    .replaceAll(" \\]", "\\]")
    .replaceAll(" \\(", "\\(")
    .replaceAll(" \\)", "\\)")
    .replaceAll(" \\.\\.\\.", "\\.\\.\\.")
    .replaceAll(" <", "<")
    .replaceAll("< ", "<")
    .replaceAll(" >", ">")
    .replaceAll(" \\?", "\\?")
    .replaceAll(", ", ",")
    .replaceAll(",", ", ");
    StringBuffer sb = new StringBuffer();
    StringTokenizer st = new StringTokenizer(prototype, ", <>[])", true);
    List tokenList = new ArrayList();
    while(st.hasMoreTokens()) {
      tokenList.add(st.nextToken());
    }
    for(int i=0; i<tokenList.size(); i++) {
      String token = (String)tokenList.get(i);
      if(token.length() > 0) {
        boolean isAdded = false;
        if(token.length() == 1) {
          switch(token.charAt(0)) {
            case ',':
            case ' ':
            case '<':
            case '>':
            case ')':
            case '[':
            case ']':
              isAdded = true;
              sb.append(token);
              break;
          }
        }
        if(!isAdded) {
          int index = token.indexOf('(');
          if(index != -1) {
            sb.append(token.substring(0, index + 1));
            tokenList.add(i + 1, token.substring(index + 1));
          } else if(Util.isReservedKeyword(token)) {
            sb.append(token);
          } else {
            boolean isVarArgs = token.endsWith("...");
            if(isVarArgs) {
              sb.append(getFullyQualifiedName(token.substring(0, token.length() - 3), imports));
              sb.append("...");
            } else {
              // TODO: If token is the name of a parameter, do not qualify it!
              if(token.startsWith("@")) {
                sb.append("@");
                token = token.substring(1);
              }
              sb.append(getFullyQualifiedName(token, imports));
            }
          }
        }
      }
    }
    String method = sb.toString();
    for(int index; (index = method.indexOf("[])")) != -1; ) {
      int spaceIndex = method.substring(0, index).lastIndexOf(' ');
      method = method.substring(0, spaceIndex) + "[]" + method.substring(spaceIndex, index) + method.substring(index + 2);
    }
    for(int index; (index = method.indexOf("[],")) != -1; ) {
      int spaceIndex = method.substring(0, index).lastIndexOf(' ');
      method = method.substring(0, spaceIndex) + "[]" + method.substring(spaceIndex, index) + method.substring(index + 2);
    }
    return method;
  }
  
  protected static String adjustField(String prototype, String[] imports) {
    prototype = prototype.replaceAll(" \\[", "\\[")
                         .replaceAll(" \\]", "\\]")
                         .replaceAll(" <", "<")
                         .replaceAll("< ", "<")
                         .replaceAll(" >", ">")
                         .replaceAll(" \\?", "\\?")
                         .replaceAll(", ", ",")
                         .replaceAll(",", ", ");
    StringBuffer sb = new StringBuffer();
    StringTokenizer st = new StringTokenizer(prototype, ", <>[]", true);
    int count = st.countTokens();
    for(int i=0; i<count - 1; i++) {
      String token = st.nextToken();
      if(token.length() > 0) {
        boolean isAdded = false;
        if(token.length() == 1) {
          switch(token.charAt(0)) {
            case ',':
            case ' ':
            case '<':
            case '>':
              isAdded = true;
              sb.append(token);
              break;
          }
        }
        if(!isAdded) {
          if(Util.isReservedKeyword(token)) {
            sb.append(token);
          } else {
            // TODO: If the token is the parameter name, do not qualify it!
            if(token.startsWith("@")) {
              sb.append("@");
              token = token.substring(1);
            }
            sb.append(getFullyQualifiedName(token, imports));
          }
        }
      }
    }
    sb.append(st.nextToken());
    String field = sb.toString();
    while(field.endsWith("[]")) {
      int index = field.lastIndexOf(' ');
      field = field.substring(0, index) + "[]" + field.substring(index, field.length() - 2);
    }
    return field;
  }

  protected static String getFullyQualifiedName(String className, String[] imports) {
    if(Util.isReservedKeyword(className) || "?".equals(className)) {
      return className;
    }
    // Problem: what about a fully qualified name to a class?
    // Problem: what about a fully qualified name to an inner class?
    // Problem: what about a name to class declared in the scope?
    System.err.println(className);
    if(className.equals("init")) {
      System.err.println("in");
    }
    if(className.indexOf('.') != -1) {
      return className;
    }
    for(int i=0; i<imports.length; i++) {
      String imp = imports[i];
      if(imp.equals(className)) {
        return imp;
      } else if(imp.endsWith("." + className)) {
        return imp;
      }
    }
    return className;
  }

  protected Map packageToClassesMap = new HashMap();
  protected final Object PACKAGE_LOCK = new Object();

  protected String[] resolveImports(List importList) {
    List newImportList = new ArrayList();
    for(int i=0; i<importList.size(); i++) {
      String className = (String)importList.get(i);
      if(className.endsWith(".*")) {
        String parentPackage = className.substring(0, className.length() - 2);
        synchronized (PACKAGE_LOCK) {
          List classList = (List)packageToClassesMap.get(parentPackage);
          if(classList == null) {
            classList = new ArrayList();
            packageToClassesMap.put(parentPackage, classList);
            classList.addAll(resolveImport(parentPackage, getSourcePath(), ".java"));
            classList.addAll(resolveImport(parentPackage, getClassPath(), ".class"));
          }
          newImportList.addAll(classList);
        }
      } else {
        newImportList.add(className);
      }
    }
    return (String[])newImportList.toArray(new String[0]);
  }

  protected Map zipPackageToImportListMap = new HashMap();

  protected List resolveImport(String parentPackage, String resourcePath, final String extension) {
    List importList = new ArrayList();
    String parentDirectory = (parentPackage + '.').replace('.', '/');
    String[] paths = resourcePath.split(Util.getPathSeparator());
    for(int i=0; i<paths.length; i++) {
      String path = paths[i];
      if(path.length() > 0) {
        File file = new File(path);
        if(file.exists()) {
          if(file.isFile()) {
            Object key = file.getAbsolutePath() + extension + "?";
            Object value = zipPackageToImportListMap.get(key);
            if(value == null) {
              zipPackageToImportListMap.put(key + parentPackage, new ArrayList());
              try {
                ZipInputStream zin = new ZipInputStream(new BufferedInputStream(new FileInputStream(file)));
                for(ZipEntry entry; (entry = zin.getNextEntry()) != null; ) {
                  String entryName = entry.getName();
                  if(entryName.endsWith(extension)) {
                    int slashIndex = entryName.lastIndexOf('/');
                    String packageName = slashIndex == -1? "": entryName.substring(0, slashIndex).replace('/', '.');
                    List zipImportList = (List)zipPackageToImportListMap.get(key + packageName);
                    if(zipImportList == null) {
                      zipImportList = new ArrayList();
                      zipPackageToImportListMap.put(key + packageName, zipImportList);
                    }
                    String className = entryName.substring(slashIndex + 1);
                    className = className.substring(0, className.length() - extension.length());
                    zipImportList.add(parentPackage.length() > 0? parentPackage + "." + className: className);
                  }
                }
              } catch(Exception e) {
                e.printStackTrace();
              }
              List zipImportList = (List)zipPackageToImportListMap.get(key + parentPackage);
              if(zipImportList != null) {
                importList.addAll(zipImportList);
              }
            }
          } else {
            try {
              File f = new File(parentDirectory);
              if(f.isDirectory()) {
                String[] files = f.list(new FilenameFilter() {
                  public boolean accept(File dir, String name) {
                    return new File(dir, name).isFile() && name.endsWith(extension);
                  }
                });
                for(int j=0; j<files.length; j++) {
                  importList.add(files[j]);
                }
              }
            } catch(Exception e) {
//            e.printStackTrace();
            }
          }
        }
      }
    }
    return importList;
  }
  
  public String getSourcePath() {
    return sourcePath == null? "": sourcePath;
  }

  public void setSourcePath(String sourcePath) {
    this.sourcePath = sourcePath;
    synchronized (PACKAGE_LOCK) {
      packageToClassesMap = new HashMap();
    }
  }

  public String getClassPath() {
    return classPath == null? "": classPath;
  }

  public void setClassPath(String classPath) {
    this.classPath = classPath;
    synchronized(PACKAGE_LOCK) {
      packageToClassesMap = new HashMap();
    }
  }

  public String getXMLDescription() {
    StringBuffer sb = new StringBuffer();
    sb.append("<sourceClassProcessorParameter>");
    String pathSeparator = Util.getPathSeparator();
    sb.append("<sourcePaths>");
    String[] sourcePaths = getSourcePath().split(pathSeparator);
    for (int i = 0; i < sourcePaths.length; i++) {
      sb.append("<sourcePath value=\"").append(Util.escapeXML(sourcePaths[i])).append("\"/>");
    }
    sb.append("</sourcePaths>");
    sb.append("<classPaths>");
    String[] classPaths = getClassPath().split(pathSeparator);
    for (int i = 0; i < classPaths.length; i++) {
      sb.append("<classPath value=\"").append(Util.escapeXML(classPaths[i])).append("\"/>");
    }
    sb.append("</classPaths>");
    sb.append("</sourceClassProcessorParameter>");
    return sb.toString();
  }

  public void loadXMLDescription(String xmlDescription) {
    try {
      DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
      DocumentBuilder builder = factory.newDocumentBuilder();
      Document document = builder.parse(new ByteArrayInputStream(xmlDescription.getBytes("UTF-8")));
      NodeList nodeList = document.getChildNodes().item(0).getChildNodes();
      for(int i=0; i<nodeList.getLength(); i++) {
        Node node = nodeList.item(i);
        String name = node.getNodeName();
        if("sourcePaths".equals(name)) {
          loadSourcePaths(node.getChildNodes());
        }
        if("classPaths".equals(name)) {
          loadClassPaths(node.getChildNodes());
        }
      }
    } catch(Exception e) {
      e.printStackTrace();
    }
  }
  
  protected void loadSourcePaths(NodeList nodeList) {
    sourcePath = null;
    StringBuffer sb = new StringBuffer();
    String pathSeparator = Util.getPathSeparator();
    for(int i=0; i<nodeList.getLength(); i++) {
      Node node = nodeList.item(i);
      String name = node.getNodeName();
      if("sourcePath".equals(name)) {
        if(i > 0) {
          sb.append(pathSeparator);
        }
        sb.append(node.getAttributes().getNamedItem("value").getNodeValue());
      }
    }
    setSourcePath(sb.toString());
  }

  protected void loadClassPaths(NodeList nodeList) {
    classPath = null;
    StringBuffer sb = new StringBuffer();
    String pathSeparator = Util.getPathSeparator();
    for(int i=0; i<nodeList.getLength(); i++) {
      Node node = nodeList.item(i);
      String name = node.getNodeName();
      if("classPath".equals(name)) {
        if(i > 0) {
          sb.append(pathSeparator);
        }
        sb.append(node.getAttributes().getNamedItem("value").getNodeValue());
      }
    }
    setClassPath(sb.toString());
  }

  protected DefaultMutableTreeNode javadocNode;
  
  public void loadClassBrowser(final JTree tree) {
    if(javadocNode != null) {
      tree.setModel(new DefaultTreeModel(javadocNode));
      return;
    }
    final String classPath = this.classPath;
    final String sourcePath = this.sourcePath;
    final DefaultMutableTreeNode node = (DefaultMutableTreeNode)tree.getModel().getRoot();
    node.removeAllChildren();
    new Thread() {
      public void run() {
        addTreeClassResources(getSourcePath(), ".java", node);
        addTreeClassResources(getClassPath(), ".class", node);
        SwingUtilities.invokeLater(new Runnable() {
          public void run() {
            if(classPath == null? SourceClassProcessor.this.classPath != null: !classPath.equals(SourceClassProcessor.this.classPath)) {
              return;
            }
            if(sourcePath == null? SourceClassProcessor.this.sourcePath != null: !sourcePath.equals(SourceClassProcessor.this.sourcePath)) {
              return;
            }
            javadocNode = node;
            tree.setModel(new DefaultTreeModel(javadocNode));
          }
        });
      }
    }.start();
  }

  protected static void addTreeClassResources(String resourcePath, String resourceExtension, DefaultMutableTreeNode node) {
    String[] paths = resourcePath.split(Util.getPathSeparator());
    for(int i=0; i<paths.length; i++) {
      String path = paths[i];
      File file = new File(path);
      if(file.exists()) {
        if(file.isFile()) {
          try {
            ZipInputStream zin = new ZipInputStream(new BufferedInputStream(new FileInputStream(file)));
            for(ZipEntry entry; (entry = zin.getNextEntry()) != null; ) {
              String name = entry.getName();
              if(name.endsWith(resourceExtension)) {
                addTreeClass(name.substring(0, name.length() - resourceExtension.length()).replace('/', '.'), node);
              }
            }
          } catch(Exception e) {
            e.printStackTrace();
          }
        } else {
          addDirectoryContent(file, null, resourceExtension, node);
        }
      }
    }
  }

  protected static void addDirectoryContent(File directory, String currentPath, String resourceExtension, DefaultMutableTreeNode node) {
    File[] files = directory.listFiles();
    for(int i=0; i<files.length; i++) {
      File file = files[i];
      String name = file.getName();
      if(file.isDirectory()) {
        addDirectoryContent(file, currentPath == null? name: currentPath + "." + name, resourceExtension, node);
      } else if(name.endsWith(resourceExtension)) {
        name = name.substring(0, name.length() - resourceExtension.length());
        addTreeClass(currentPath == null? name: currentPath + "." + name, node);
      }
    }
  }

  protected static void addTreeClass(String className, DefaultMutableTreeNode node) {
    int index = className.indexOf('.');
    if(index != -1) {
      String packageName = className.substring(0, index);
      for(int i=node.getChildCount()-1; i>=0; i--) {
        DefaultMutableTreeNode childNode = (DefaultMutableTreeNode)node.getChildAt(i);
        String childPackageOrClassName = ((String)childNode.getUserObject());
        int comparison = childNode.isLeaf()? 1: childPackageOrClassName.compareTo(packageName);
        if(comparison == 0) {
          addTreeClass(className.substring(index + 1), childNode);
          return;
        }
        if(comparison < 0) {
          childNode = new DefaultMutableTreeNode(packageName);
          node.insert(childNode, i + 1);
          addTreeClass(className.substring(index + 1), childNode);
          return;
        }
      }
      DefaultMutableTreeNode childNode = new DefaultMutableTreeNode(packageName);
      node.insert(childNode, 0);
      addTreeClass(className.substring(index + 1), childNode);
      return;
    }
    for(int i=node.getChildCount()-1; i>=0; i--) {
      DefaultMutableTreeNode childNode = (DefaultMutableTreeNode)node.getChildAt(i);
      String childClassName = ((String)childNode.getUserObject());
      int comparison = !childNode.isLeaf()? -1: childClassName.compareTo(className);
      if(comparison == 0) {
        return;
      }
      if(comparison < 0) {
        childNode = new DefaultMutableTreeNode(Util.unescapeClassName(className));
        node.insert(childNode, i + 1);
        return;
      }
    }
    node.insert(new DefaultMutableTreeNode(Util.unescapeClassName(className)), 0);
  }

}
