/*
 * Christopher Deckers (chrriis@nextencia.net)
 * http://www.nextencia.net
 * 
 * See the file "readme.txt" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
package chrriis.udoc.model;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.util.ArrayList;
import java.util.List;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.transform.Result;
import javax.xml.transform.Source;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;

import org.w3c.dom.Document;
import org.w3c.dom.NamedNodeMap;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

import chrriis.udoc.model.processor.ClassProcessor;
import chrriis.udoc.model.processor.ClassProcessorRegistry;

public class ModelDescriptor {

  public static String getXMLDescription() {
    StringBuffer sb = new StringBuffer();
    sb.append("<model>");
    sb.append("<classProcessors>");
    ClassProcessor[] classProcessors = ClassProcessorRegistry.getClassProcessors();
    for(int i=0; i<classProcessors.length; i++) {
      ClassProcessor classProcessor = classProcessors[i];
      sb.append("<classProcessor id=\"").append(Util.escapeXML(classProcessor.getProcessorID())).append("\">");
      String xmlDescription = classProcessor.getXMLDescription();
      if(xmlDescription != null) {
        sb.append(xmlDescription);
      }
      sb.append("</classProcessor>");
    }
    sb.append("</classProcessors>");
    sb.append("<classes>");
    ClassInfo[] classInfos = ClassInfoLoader.getClassInfos();
    for(int i=0; i<classInfos.length; i++) {
      ClassInfo classInfo = classInfos[i];
      sb.append(getClassDescription(classInfo));
    }
    sb.append("</classes>");
    sb.append("</model>");
    return sb.toString();
  }

  protected static String getClassDescription(ClassInfo classInfo) {
    StringBuffer sb = new StringBuffer();
    int loadingState = classInfo.getLoadingState();
    ClassProcessor classProcessor = classInfo.getClassProcessor();
    String classProcessorID = classProcessor == null? "": classProcessor.getProcessorID();
    sb.append("<class name=\"").append(Util.escapeXML(classInfo.getClassName())).append("\" state=\"").append(getLoadingStateDescription(loadingState)).append("\" classProcessor=\"").append(classProcessorID).append("\">");
    if(loadingState == ClassInfo.LOADED_STATE) {
      sb.append("<prototype value=\"").append(Util.escapeXML(classInfo.getPrototype())).append("\"/>");
      ClassInfo[] superTypes = classInfo.getSuperTypes();
      sb.append("<superTypes>");
      for(int i=0; i<superTypes.length; i++) {
        sb.append("<superType value=\"").append(Util.escapeXML(createSimplePrototype(superTypes[i]))).append("\"/>");
      }
      sb.append("</superTypes>");
      ClassInfo[] subTypes = classInfo.getSubTypes();
      sb.append("<subTypes>");
      for(int i=0; i<subTypes.length; i++) {
        sb.append("<subType value=\"").append(Util.escapeXML(createSimplePrototype(subTypes[i]))).append("\"/>");
      }
      sb.append("</subTypes>");
      FieldInfo[] enums = classInfo.getEnums();
      sb.append("<enums>");
      for(int i=0; i<enums.length; i++) {
        sb.append("<enum value=\"").append(Util.escapeXML(enums[i].getPrototype())).append("\"/>");
      }
      sb.append("</enums>");
      FieldInfo[] fields = classInfo.getFields();
      sb.append("<fields>");
      for(int i=0; i<fields.length; i++) {
        sb.append("<field value=\"").append(Util.escapeXML(fields[i].getPrototype())).append("\"/>");
      }
      sb.append("</fields>");
      MethodInfo[] constructors = classInfo.getConstructors();
      sb.append("<constructors>");
      for(int i=0; i<constructors.length; i++) {
        sb.append("<constructor value=\"").append(Util.escapeXML(constructors[i].getPrototype())).append("\"/>");
      }
      sb.append("</constructors>");
      MethodInfo[] methods = classInfo.getMethods();
      sb.append("<methods>");
      for(int i=0; i<methods.length; i++) {
        sb.append("<method value=\"").append(Util.escapeXML(methods[i].getPrototype())).append("\"/>");
      }
      sb.append("</methods>");
      MethodInfo[] annotationMembers = classInfo.getAnnotationMembers();
      sb.append("<annotationMembers>");
      for(int i=0; i<annotationMembers.length; i++) {
        sb.append("<annotationMember value=\"").append(Util.escapeXML(annotationMembers[i].getPrototype())).append("\"/>");
      }
      sb.append("</annotationMembers>");
    }
    sb.append("</class>");
    return sb.toString();
  }
  
  protected static String createSimplePrototype(ClassInfo classInfo) {
    return Util.getModifiers(classInfo.getModifiers()) + classInfo.getDeclaration();
  }
  
  protected static String getLoadingStateDescription(int loadingState) {
    switch(loadingState) {
      case ClassInfo.LOADED_STATE: return "loaded";
      case ClassInfo.LOADING_FAILED_STATE: return "failed";
    }
    return "notLoaded";
  }
  
  protected static int getLoadingState(String loadingStateDescription) {
    if("loaded".equals(loadingStateDescription)) {
      return ClassInfo.LOADED_STATE;
    }
    if("failed".equals(loadingStateDescription)) {
      return ClassInfo.LOADING_FAILED_STATE;
    }
    return ClassInfo.NOT_LOADED_STATE;
  }
  
  public static void clearModel() {
    setXMLDescription("<model><classProcessors></classProcessors><classes></classes></model>");
  }
  
  public static void setXMLDescription(String description) {
    try {
      DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
      DocumentBuilder builder = factory.newDocumentBuilder();
      Document document = builder.parse(new ByteArrayInputStream(description.getBytes("UTF-8")));
      NodeList nodeList = document.getChildNodes().item(0).getChildNodes();
      for(int i=0; i<nodeList.getLength(); i++) {
        Node node = nodeList.item(i);
        String name = node.getNodeName();
        if("classProcessors".equals(name)) {
          loadClassProcessors(node.getChildNodes());
        } else if("classes".equals(name)) {
          loadClasses(node.getChildNodes());
        }
      }
    } catch(Exception e) {
      e.printStackTrace();
    }
  }

  protected static void loadClassProcessors(NodeList nodeList) {
    for(int i=0; i<nodeList.getLength(); i++) {
      Node node = nodeList.item(i);
      String name = node.getNodeName();
      if("classProcessor".equals(name)) {
        String id = node.getAttributes().getNamedItem("id").getNodeValue();
        ClassProcessor classProcessor = ClassProcessorRegistry.getClassProcessor(id);
        if(classProcessor != null) {
          NodeList childNodeList =  node.getChildNodes();
          StringBuffer sb = new StringBuffer();
          for(int j=0; j<childNodeList.getLength(); j++) {
            try {
              ByteArrayOutputStream out = new ByteArrayOutputStream();
              Source source = new DOMSource(childNodeList.item(j));
              Result result = new StreamResult(out);
              Transformer transformer = TransformerFactory.newInstance().newTransformer();
              transformer.transform(source, result);
              String textContent = new String(out.toByteArray(), "UTF-8");
              if(textContent != null) {
                sb.append(textContent);
              }
            }
            catch (Exception e) {
              e.printStackTrace();
            }
          }
          classProcessor.loadXMLDescription(sb.toString());
        }
      }
      
    }
  }

  protected static void loadClasses(NodeList nodeList) {
    List classInfoList = new ArrayList();
    List classDescriptionList = new ArrayList();
    for(int i=0; i<nodeList.getLength(); i++) {
      Node node = nodeList.item(i);
      String name = node.getNodeName();
      if("class".equals(name)) {
        NamedNodeMap attributes = node.getAttributes();
        String className = attributes.getNamedItem("name").getNodeValue();
        String loadingStateDescription = attributes.getNamedItem("state").getNodeValue();
        String classProcessor = attributes.getNamedItem("classProcessor").getNodeValue();
        ClassInfo classInfo = new ClassInfo(className, ClassProcessorRegistry.getClassProcessor(classProcessor));
        int loadingState = getLoadingState(loadingStateDescription);
        classInfo.setLoadingState(loadingState);
        classInfoList.add(classInfo);
        byte[] description = null;
        if(loadingState == ClassInfo.LOADED_STATE) {
          try {
            ByteArrayOutputStream out = new ByteArrayOutputStream();
            Source source = new DOMSource(node);
            Result result = new StreamResult(out);
            Transformer transformer = TransformerFactory.newInstance().newTransformer();
            transformer.transform(source, result);
            description = out.toByteArray();
          }
          catch (Exception e) {
            e.printStackTrace();
          }
        }
        classDescriptionList.add(description);
      }
    }
    ClassInfo[] classInfos = (ClassInfo[])classInfoList.toArray(new ClassInfo[0]);
    ClassInfoLoader.setClassInfos(classInfos);
    for(int i=0; i<classInfos.length; i++) {
      ClassInfo classInfo = classInfos[i];
      byte[] classData = (byte[])classDescriptionList.get(i);
      if(classData != null) {
        ClassInfoLoader.loadClassInfo(classInfo, classInfo.getClassProcessor(), new ByteArrayInputStream(classData));
      }
    }
  }

}
