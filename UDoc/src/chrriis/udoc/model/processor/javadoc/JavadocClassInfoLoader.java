/*
 * Christopher Deckers (chrriis@nextencia.net)
 * http://www.nextencia.net
 * 
 * See the file "readme.txt" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
package chrriis.udoc.model.processor.javadoc;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.net.URL;
import java.util.HashMap;

import chrriis.udoc.model.ClassInfo;
import chrriis.udoc.model.Modifiers;
import chrriis.udoc.model.URLConnectionEx;
import chrriis.udoc.model.Util;

public class JavadocClassInfoLoader {

  protected static final int UNDEFINED = 0;
  protected static final int CLASS_DATA_START = 1;
  protected static final int ENUM_CONSTANT_SUMMARY = 2;
  protected static final int ANNOTATION_REQUIRED_MEMBER_SUMMARY = 3;
  protected static final int FIELD_SUMMARY = 4;
  protected static final int CONSTRUCTOR_SUMMARY = 5;
  protected static final int METHOD_SUMMARY = 6;
  protected static final int ENUM_CONSTANT_DETAIL = 7;
  protected static final int ANNOTATION_MEMBER_DETAIL = 8;
  protected static final int FIELD_DETAIL = 9;
  protected static final int CONSTRUCTOR_DETAIL = 10;
  protected static final int METHOD_DETAIL = 11;
  protected static final int CLASS_DATA_END = 12;

  protected static final String CLASS_DATA_START_TEXT = "<!-- ======== START OF CLASS DATA ======== -->";
  protected static final String ENUM_CONSTANT_SUMMARY_TEXT = "<!-- =========== ENUM CONSTANT SUMMARY =========== -->";
  protected static final String ANNOTATION_REQUIRED_MEMBER_SUMMARY_TEXT = "<!-- =========== ANNOTATION TYPE REQUIRED MEMBER SUMMARY =========== -->";
  protected static final String FIELD_SUMMARY_TEXT = "<!-- =========== FIELD SUMMARY =========== -->";
  protected static final String CONSTRUCTOR_SUMMARY_TEXT = "<!-- ======== CONSTRUCTOR SUMMARY ======== -->";
  protected static final String METHOD_SUMMARY_TEXT = "<!-- ========== METHOD SUMMARY =========== -->";
  protected static final String ENUM_CONSTANT_DETAIL_TEXT = "<!-- ============ ENUM CONSTANT DETAIL =========== -->";
  protected static final String ANNOTATION_MEMBER_DETAIL_TEXT = "<!-- ============ ANNOTATION TYPE MEMBER DETAIL =========== -->";
  protected static final String FIELD_DETAIL_TEXT = "<!-- ============ FIELD DETAIL =========== -->";
  protected static final String CONSTRUCTOR_DETAIL_TEXT = "<!-- ========= CONSTRUCTOR DETAIL ======== -->";
  protected static final String METHOD_DETAIL_TEXT = "<!-- ============ METHOD DETAIL ========== -->";
  protected static final String CLASS_DATA_END_TEXT = "<!-- ========= END OF CLASS DATA ========= -->";

  public static String loadClassData(String className, String documentationRoot) {
//    String subFolder = classInfo.getPackage().getName().replace('.', '/');
    int lastDotIndex = className.lastIndexOf('.');
//    String name = Utils.unescapeClassName(className.substring(lastDotIndex + 1));
    String packageName = lastDotIndex == -1? "": className.substring(0, lastDotIndex);
    String docPath = Util.unescapeClassName(className.replace('.', '/')) + ".html";
    if(documentationRoot != null) {
      docPath = documentationRoot + "/" + docPath;
    }
//    String classDescription = null;
    StringBuffer sb = new StringBuffer();
    sb.append("<type name=\"").append(Util.escapeXML(className)).append("\">");
    try {
      BufferedReader reader = new BufferedReader(new InputStreamReader(URLConnectionEx.openConnection(new URL(docPath)).getInputStream()));
      int state = UNDEFINED;
      boolean hasEnumDetail = false;
      boolean hasFieldDetail = false;
      boolean hasConstructorDetail = false;
      boolean hasMethodDetail = false;
      StringBuffer subTypeSB = new StringBuffer();
      int classModifiers = 0;
      for(String line; (line = reader.readLine()) != null; ) {
        line = line.trim();
        switch(state) {
          case UNDEFINED:
            if(line.startsWith(CLASS_DATA_START_TEXT)) {
              state = changeState(state, CLASS_DATA_START, sb);
            }
            break;
          case CLASS_DATA_START:
            if(line.startsWith(ENUM_CONSTANT_SUMMARY_TEXT)) {state = changeState(state, ENUM_CONSTANT_SUMMARY, sb); hasEnumDetail = true; break;}
            if(line.startsWith(ANNOTATION_REQUIRED_MEMBER_SUMMARY_TEXT)) {state = changeState(state, ANNOTATION_REQUIRED_MEMBER_SUMMARY, sb); break;}
            if(line.startsWith(FIELD_SUMMARY_TEXT)) {state = changeState(state, FIELD_SUMMARY, sb); hasFieldDetail = true; break;}
            if(line.startsWith(CONSTRUCTOR_SUMMARY_TEXT)) {state = changeState(state, CONSTRUCTOR_SUMMARY, sb); hasConstructorDetail = true; break;}
            if(line.startsWith(METHOD_SUMMARY_TEXT)) {state = changeState(state, METHOD_SUMMARY, sb); hasMethodDetail = true; break;}
            if(line.startsWith(CLASS_DATA_END_TEXT)) {state = changeState(state, CLASS_DATA_END, sb); break;}
            if(line.startsWith("<DT><B>All Known ") || line.startsWith("<DT><B>Direct Known ")) {
//              for(String nextLine; (nextLine = reader.readLine().trim()).indexOf("</DL>") == -1; ) {
//                line += " " + nextLine;
//              }
              String type = line.startsWith("All Known Subinterfaces")? "interface ": "class ";
              line = line.substring(line.indexOf("<DD>") + "<DD>".length());
              String[] tokens = line.split(", ");
              for(int i=0; i<tokens.length; i++) {
                String token = tokens[i].trim().replaceAll("</DD>", "");
                subTypeSB.append("<subType value=\"" + type + processType(token, documentationRoot, packageName) + "\"/>");
              }
              break;
            }
            if(line.startsWith("<DT><PRE>")) {
              while(!line.contains("</DL>")) {
                line = line + " " + reader.readLine();
              }
              line = line.substring("<DT><PRE>".length());
              String[] tokens = line.split("<DT>");
              int index = line.indexOf("<B>");
              if(index != -1) {
                index = index + "<B>".length();
                line = line.substring(0, index) + packageName + '.' + line.substring(index);
              }
              String prototypeToken = tokens[0].replaceAll("<B>", "").replaceAll("</B>", "").replaceAll("</DL>", "");
//              String prototype = removeTags(processType(prototypeToken, documentationRoot, packageName));
//              String oldName = Utils.unescapeClassName(className.substring(className.lastIndexOf('.') + 1));
//              int lastIndex = prototype.lastIndexOf(oldName);
//              prototype = prototype.substring(0, lastIndex) + className + prototype.substring(lastIndex + oldName.length());
              String prototype = removeTags(line.replaceAll("<DT>", " "));
              sb.append("<prototype value=\"" + prototype + "\"/>");
              classModifiers = Util.getModifiers(prototypeToken);
              sb.append("<superTypes>");
              for(int i=1; i<tokens.length; i++) {
                String superTypes = tokens[i];
                int modifier = 0;
                if(superTypes.startsWith("implements ")) {
                  superTypes = superTypes.substring("implements ".length());
                  modifier = Modifiers.INTERFACE;
                } else if(superTypes.startsWith("extends ")) {
                  superTypes = superTypes.substring("extends ".length());
                  modifier = (classModifiers & Modifiers.INTERFACE) != 0? Modifiers.INTERFACE: Modifiers.CLASS;
                }
                String[] superTypeTokens = superTypes.split(", ");
                for(int j=0; j<superTypeTokens.length; j++) {
                  String token = superTypeTokens[j].trim().replaceAll("</DL>", "");
                  sb.append("<superType value=\"");
                  if((modifier & Modifiers.CLASS) != 0) {
                    sb.append("class ");
                  } else if((modifier & Modifiers.ANNOTATION) != 0) {
                    sb.append("@interface ");
                  } else if((modifier & Modifiers.ENUM) != 0) {
                    sb.append("enum ");
                  } else if((modifier & Modifiers.INTERFACE) != 0) {
                    sb.append("interface ");
                  }
                  sb.append(processType(token, documentationRoot, packageName)).append("\"/>");
                }
              }
              sb.append("</superTypes>");
              sb.append("<subTypes>" + subTypeSB.toString() + "</subTypes>");
//            } else if(classDescription == null && "<P>".equals(line)) {
//              StringBuffer descriptionSB = new StringBuffer();
//              while(!"<P>".equals(line = reader.readLine())) {
//                descriptionSB.append(line);
//              }
//              classDescription = descriptionSB.toString();
            }
            break;
          case ENUM_CONSTANT_SUMMARY:
            if(line.startsWith(ANNOTATION_REQUIRED_MEMBER_SUMMARY_TEXT)) {state = changeState(state, ANNOTATION_REQUIRED_MEMBER_SUMMARY, sb); break;}
            if(line.startsWith(FIELD_SUMMARY_TEXT)) {state = changeState(state, FIELD_SUMMARY, sb); hasFieldDetail = true; break;}
            if(line.startsWith(CONSTRUCTOR_SUMMARY_TEXT)) {state = changeState(state, CONSTRUCTOR_SUMMARY, sb); hasConstructorDetail = true; break;}
            if(line.startsWith(METHOD_SUMMARY_TEXT)) {state = changeState(state, METHOD_SUMMARY, sb); hasMethodDetail = true; break;}
            if(line.startsWith(ENUM_CONSTANT_DETAIL_TEXT)) {state = changeState(state, ENUM_CONSTANT_DETAIL, sb); break;}
            if(line.startsWith(FIELD_DETAIL_TEXT)) {state = changeState(state, FIELD_DETAIL, sb); break;}
            if(line.startsWith(CLASS_DATA_END_TEXT)) {state = changeState(state, CLASS_DATA_END, sb); break;}
            break;
          case ANNOTATION_REQUIRED_MEMBER_SUMMARY:
            if(line.startsWith(FIELD_SUMMARY_TEXT)) {state = changeState(state, FIELD_SUMMARY, sb); hasFieldDetail = true; break;}
            if(line.startsWith(CONSTRUCTOR_SUMMARY_TEXT)) {state = changeState(state, CONSTRUCTOR_SUMMARY, sb); hasConstructorDetail = true; break;}
            if(line.startsWith(METHOD_SUMMARY_TEXT)) {state = changeState(state, METHOD_SUMMARY, sb); hasMethodDetail = true; break;}
            if(hasEnumDetail && line.startsWith(ENUM_CONSTANT_DETAIL_TEXT)) {state = changeState(state, ENUM_CONSTANT_DETAIL, sb); break;}
            if(line.startsWith(ANNOTATION_MEMBER_DETAIL_TEXT)) {state = changeState(state, ANNOTATION_MEMBER_DETAIL, sb); break;}
            if(line.startsWith(FIELD_DETAIL_TEXT)) {state = changeState(state, FIELD_DETAIL, sb); break;}
            if(line.startsWith(CLASS_DATA_END_TEXT)) {state = changeState(state, CLASS_DATA_END, sb); break;}
            break;
          case FIELD_SUMMARY:
            if(line.startsWith(CONSTRUCTOR_SUMMARY_TEXT)) {state = changeState(state, CONSTRUCTOR_SUMMARY, sb); hasConstructorDetail = true; break;}
            if(line.startsWith(METHOD_SUMMARY_TEXT)) {state = changeState(state, METHOD_SUMMARY, sb); hasMethodDetail = true; break;}
            if(hasEnumDetail && line.startsWith(ENUM_CONSTANT_DETAIL_TEXT)) {state = changeState(state, ENUM_CONSTANT_DETAIL, sb); break;}
            if(line.startsWith(ANNOTATION_MEMBER_DETAIL_TEXT)) {state = changeState(state, ANNOTATION_MEMBER_DETAIL, sb); break;}
            if(line.startsWith(FIELD_DETAIL_TEXT)) {state = changeState(state, FIELD_DETAIL, sb); break;}
            if(line.startsWith(CLASS_DATA_END_TEXT)) {state = changeState(state, CLASS_DATA_END, sb); break;}
            break;
          case CONSTRUCTOR_SUMMARY:
            if(line.indexOf(METHOD_SUMMARY_TEXT) >= 0) {state = changeState(state, METHOD_SUMMARY, sb); hasMethodDetail = true; break;}
            if(hasEnumDetail && line.startsWith(ENUM_CONSTANT_DETAIL_TEXT)) {state = changeState(state, ENUM_CONSTANT_DETAIL, sb); break;}
            if(hasFieldDetail && line.startsWith(FIELD_DETAIL_TEXT)) {state = changeState(state, FIELD_DETAIL, sb); break;}
            if(line.startsWith(CONSTRUCTOR_DETAIL_TEXT)) {state = changeState(state, CONSTRUCTOR_DETAIL, sb); break;}
            if(line.startsWith(CLASS_DATA_END_TEXT)) {state = changeState(state, CLASS_DATA_END, sb); break;}
            break;
          case METHOD_SUMMARY:
            if(hasEnumDetail && line.startsWith(ENUM_CONSTANT_DETAIL_TEXT)) {state = changeState(state, ENUM_CONSTANT_DETAIL, sb); break;}
            if(hasFieldDetail && line.startsWith(FIELD_DETAIL_TEXT)) {state = changeState(state, FIELD_DETAIL, sb); break;}
            if(hasConstructorDetail && line.startsWith(CONSTRUCTOR_DETAIL_TEXT)) {state = changeState(state, CONSTRUCTOR_DETAIL, sb); break;}
            if(line.startsWith(METHOD_DETAIL_TEXT)) {state = changeState(state, METHOD_DETAIL, sb); break;}
            if(line.startsWith(CLASS_DATA_END_TEXT)) {state = changeState(state, CLASS_DATA_END, sb); break;}
            break;
          case ENUM_CONSTANT_DETAIL:
            if(line.startsWith(ANNOTATION_MEMBER_DETAIL_TEXT)) {state = changeState(state, ANNOTATION_MEMBER_DETAIL, sb); break;}
            if(hasFieldDetail && line.startsWith(FIELD_DETAIL_TEXT)) {state = changeState(state, FIELD_DETAIL, sb); break;}
            if(hasConstructorDetail && line.startsWith(CONSTRUCTOR_DETAIL_TEXT)) {state = changeState(state, CONSTRUCTOR_DETAIL, sb); break;}
            if(hasMethodDetail && line.startsWith(METHOD_DETAIL_TEXT)) {state = changeState(state, METHOD_DETAIL, sb); break;}
            if(line.startsWith(CLASS_DATA_END_TEXT)) {state = changeState(state, CLASS_DATA_END, sb); break;}
            if(line.startsWith("<PRE>")) {
              line = reader.readLine().trim();
              while(!line.endsWith("</PRE>")) {
                line = line + ' ' + reader.readLine().trim();
              }
              sb.append("<enum value=\"" + removeTags(processType(line.replaceAll("</PRE>", ""), documentationRoot, packageName)) + "\"/>");
            }
            break;
          case ANNOTATION_MEMBER_DETAIL:
            if(hasFieldDetail && line.startsWith(FIELD_DETAIL_TEXT)) {state = changeState(state, FIELD_DETAIL, sb); break;}
            if(hasConstructorDetail && line.startsWith(CONSTRUCTOR_DETAIL_TEXT)) {state = changeState(state, CONSTRUCTOR_DETAIL, sb); break;}
            if(hasMethodDetail && line.startsWith(METHOD_DETAIL_TEXT)) {state = changeState(state, METHOD_DETAIL, sb); break;}
            if(line.startsWith(CLASS_DATA_END_TEXT)) {state = changeState(state, CLASS_DATA_END, sb); break;}
            if(line.startsWith("<PRE>")) {
              line = reader.readLine().trim();
              while(!line.endsWith("</PRE>")) {
                line = line + ' ' + reader.readLine().trim();
              }
              sb.append("<annotationMember value=\"" + removeTags(processType(line.replaceAll("</PRE>", ""), documentationRoot, packageName)) + "()\"/>");
            }
            break;
          case FIELD_DETAIL:
            if(hasConstructorDetail && line.startsWith(CONSTRUCTOR_DETAIL_TEXT)) {state = changeState(state, CONSTRUCTOR_DETAIL, sb); break;}
            if(hasMethodDetail && line.startsWith(METHOD_DETAIL_TEXT)) {state = changeState(state, METHOD_DETAIL, sb); break;}
            if(line.startsWith(CLASS_DATA_END_TEXT)) {state = changeState(state, CLASS_DATA_END, sb); break;}
            if(line.startsWith("<PRE>")) {
              line = reader.readLine().trim();
              while(!line.endsWith("</PRE>")) {
                line = line + ' ' + reader.readLine().trim();
              }
              sb.append("<field value=\"" + removeTags(processType(line.replaceAll("</PRE>", ""), documentationRoot, packageName)) + "\"/>");
            }
            break;
          case CONSTRUCTOR_DETAIL:
            if(hasMethodDetail && line.startsWith(METHOD_DETAIL_TEXT)) {state = changeState(state, METHOD_DETAIL, sb); break;}
            if(line.startsWith(CLASS_DATA_END_TEXT)) {state = changeState(state, CLASS_DATA_END, sb); break;}
            if(line.startsWith("<PRE>")) {
              line = reader.readLine().trim();
              while(!line.endsWith("</PRE>")) {
                line = line + ' ' + reader.readLine().trim();
              }
              sb.append("<constructor value=\"" + removeTags(processType(line.replaceAll("</PRE>", ""), documentationRoot, packageName)) + "\"/>");
            }
            break;
          case METHOD_DETAIL:
            if(line.startsWith(CLASS_DATA_END_TEXT)) {state = changeState(state, CLASS_DATA_END, sb); break;}
            if(line.startsWith("<PRE>")) {
              line = reader.readLine().trim();
              while(!line.endsWith("</PRE>")) {
                line = line + ' ' + reader.readLine().trim();
              }
              if(line.indexOf("</B>(") != -1) {
                sb.append("<method value=\"");
                if((classModifiers & Modifiers.INTERFACE) != 0) {
                  sb.append("public ");
                }
                sb.append(removeTags(processType(line.replaceAll("</PRE>", ""), documentationRoot, packageName)) + "\"/>");
              }
            }
            break;
          case CLASS_DATA_END:
            break;
        }
      }
      setPackageDocumentationRoot(packageName, documentationRoot);
    } catch(Exception e) {
      e.printStackTrace();
      return null;
    }
    sb.append("</type>");
    System.err.println(sb.toString());
    return sb.toString();
  }

  protected static int changeState(int oldState, int newState, StringBuffer sb) {
    if(oldState == newState) {
      return newState;
    }
    switch(oldState) {
      case ENUM_CONSTANT_DETAIL:
        sb.append("</enums>");
        break;
      case ANNOTATION_MEMBER_DETAIL:
        sb.append("</annotationMembers>");
        break;
      case FIELD_DETAIL:
        sb.append("</fields>");
        break;
      case METHOD_DETAIL:
        sb.append("</methods>");
        break;
      case CONSTRUCTOR_DETAIL:
        sb.append("</constructors>");
        break;
    }
    switch(newState) {
      case ENUM_CONSTANT_DETAIL:
        sb.append("<enums>");
        break;
      case ANNOTATION_MEMBER_DETAIL:
        sb.append("<annotationMembers>");
        break;
      case FIELD_DETAIL:
        sb.append("<fields>");
        break;
      case METHOD_DETAIL:
        sb.append("<methods>");
        break;
      case CONSTRUCTOR_DETAIL:
        sb.append("<constructors>");
        break;
    }
    return newState;
  }

  protected static String processType(String pType, String documentationRoot, String referencePackageName) {
    pType = pType.replaceAll("<B>", "").replaceAll("</B>", "").replaceAll("&nbsp;", " ");
    int analysisStart = 0;
    for(int index; (index=pType.indexOf("<A HREF=\"", analysisStart)) != -1; ) {
      int processIndex = index;
      String modifiers = pType.substring(0, index);
      String type = pType.substring(processIndex + "<A HREF=\"".length());
      index = type.indexOf('\"');
      String docPath = type.substring(0, index);
      int endTagIndex = type.indexOf('>', index + 1);
      index = type.indexOf(" title=\"", index + 1);
      // When the title is not found, we leave the tag untouched, which is the case for annotations' parameters for example.
      if(index != -1 && index < endTagIndex) {
        type = type.substring(index + " title=\"".length());
        if(type.startsWith("class ") || type.startsWith("annotation ") || type.startsWith("enum ") || type.startsWith("interface ")) {
          type = type.substring(type.indexOf(" in ") + " in ".length());
          index = type.indexOf('\"');
          String endString = type.substring(type.indexOf('>') + 1);
          boolean isAnnotation = endString.startsWith("@");
          if(isAnnotation) {
            endString = endString.substring(1);
          }
          int aIndex = endString.indexOf("</A>");
          endString = Util.escapeClassName(endString.substring(0, aIndex)) + endString.substring(aIndex + "</A>".length());
          String packageName = type.substring(0, index);
          String startString = modifiers + (isAnnotation? "@": "") + packageName + ".";
          analysisStart = startString.length();
          type = startString + endString;
          
          String referenceDocPath = documentationRoot;
          if(referencePackageName.length() > 0) {
            referenceDocPath += '/' + referencePackageName.replace('.', '/');
          }
          int indexOfIndex = docPath.indexOf("/index.html");
          if(indexOfIndex != -1) {
            docPath = docPath.substring(0, indexOfIndex) + '/' + packageName.replace('.', '/') + "/a.html";
          }
          if(packageName.length() > 0) {
            docPath = docPath.substring(0, docPath.lastIndexOf(packageName.replace('.', '/') + '/'));
          } else {
            docPath = docPath.substring(0, docPath.lastIndexOf('/') + 1);
          }
          while(docPath.startsWith("./")) {
            docPath = docPath.substring("./".length());
          }
          if(docPath.startsWith("../")) {
            while(docPath.startsWith("../")) {
              docPath = docPath.substring("../".length());
              referenceDocPath = referenceDocPath.substring(0, referenceDocPath.lastIndexOf('/'));
            }
            referenceDocPath = referenceDocPath + "/" + docPath;
          } else if(docPath.startsWith("/")) {
            int pIndex = referenceDocPath.indexOf("://");
            if(pIndex == -1) {
              pIndex = referenceDocPath.indexOf(":/");
              referenceDocPath = referenceDocPath.substring(0, pIndex + 1);
            } else {
              referenceDocPath = referenceDocPath.substring(0, pIndex + 2);
            }
          } else if(docPath.indexOf(":/") != -1) {
            referenceDocPath = docPath;
          } else {
            referenceDocPath = referenceDocPath + "/" + docPath;
          }
          if(referenceDocPath.endsWith("/")) {
            referenceDocPath = referenceDocPath.substring(0, referenceDocPath.length() - 1);
          }
          setPackageDocumentationRoot(packageName, referenceDocPath);
        } else {
          index = type.indexOf('\"');
          String endString = type.substring(type.indexOf('>') + 1);
          int aIndex = endString.indexOf("</A>");
          String startString = modifiers + endString.substring(0, aIndex);
          analysisStart = startString.length();
          type = startString + endString.substring(aIndex + "</A>".length());
        }
        pType = type;
      } else {
        analysisStart = modifiers.length() + 1;
      }
    }
    return pType.replaceAll(" +", " ").replaceAll(" +", " ");
  }

  protected static String removeTags(String s) {
    StringBuffer sb = new StringBuffer(s.length());
    int count = 0;
    for(int i=0; i<s.length(); i++) {
      char c = s.charAt(i);
      switch(c) {
        case '<': count++; break;
        case '>': count--; break;
        default:
          if(count == 0) {
            sb.append(c);
          }
      }
    }
    return sb.toString();
  }

  protected static HashMap packageToDocumentationRootMap = new HashMap();

  protected static void setPackageDocumentationRoot(String packageName, String documentationRoot) {
    packageToDocumentationRootMap.put(packageName, documentationRoot);
  }

  protected static void removePackageDocumentationRoot(String packageName) {
    packageToDocumentationRootMap.remove(packageName);
  }
  
  protected static String[] getPackageNames() {
    return (String[])packageToDocumentationRootMap.keySet().toArray(new String[0]);
  }

  protected static void clearPackageDocumentationRootInformation() {
    packageToDocumentationRootMap.clear();
  }

  protected static String getPackageDocumentationRoot(String packageName) {
    for(int index=packageName.length(); index>=0; ) {
      String documentationRoot = (String)packageToDocumentationRootMap.get(packageName.substring(0, index));
      if(documentationRoot != null) {
        return documentationRoot;
      }
      index = packageName.lastIndexOf('.', index - 1);
    }
    return null;
  }

  public static void destroyClassInfo(ClassInfo classInfo) {
    
  }

}
