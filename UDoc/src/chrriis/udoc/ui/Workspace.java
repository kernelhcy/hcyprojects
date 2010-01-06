/*
 * Christopher Deckers (chrriis@nextencia.net)
 * http://www.nextencia.net
 * 
 * See the file "readme.txt" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
package chrriis.udoc.ui;

import java.awt.Dimension;
import java.awt.Point;
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

import chrriis.udoc.model.ClassInfoLoader;
import chrriis.udoc.model.ModelDescriptor;
import chrriis.udoc.model.Modifiers;
import chrriis.udoc.model.Util;
import chrriis.udoc.model.processor.ClassProcessor;
import chrriis.udoc.model.processor.ClassProcessorRegistry;

public class Workspace {

  protected static String getVisibility(int visibility) {
    switch(visibility) {
    case ClassPane.PUBLIC_VISIBILITY:
      return "public";
    case ClassPane.PROTECTED_VISIBILITY:
      return "protected";
    case ClassPane.DEFAULT_VISIBILITY:
      return "default";
    }
    return "private";
  }
  
  protected static int getVisibility(String visibility) {
    if("public".equals(visibility)) {
      return ClassPane.PUBLIC_VISIBILITY;
    }
    if("protected".equals(visibility)) {
      return ClassPane.PROTECTED_VISIBILITY;
    }
    if("default".equals(visibility)) {
      return ClassPane.DEFAULT_VISIBILITY;
    }
    return ClassPane.PRIVATE_VISIBILITY;
  }
  
  public static String getXMLDescription(ClassPane classPane) {
    StringBuffer sb = new StringBuffer();
    sb.append("<workspace>");
    sb.append(ModelDescriptor.getXMLDescription());
    sb.append("<desktop>");
    ClassProcessor lastClassProcessor = classPane.getLastClassProcessor();
    if(lastClassProcessor != null) {
      sb.append("<selection classes=\"").append(Util.escapeXML(classPane.getLastClassNames())).append("\" classProcessor=\"").append(Util.escapeXML(lastClassProcessor.getProcessorID())).append("\"/>");
    }
    Filter[] filters = classPane.getFilters();
    sb.append("<filters fieldVisibility=\"").append(Util.escapeXML(getVisibility(classPane.getFieldVisibility()))).append("\" methodVisibility=\"").append(Util.escapeXML(getVisibility(classPane.getMethodVisibility()))).append("\">");
    for(int i=0; i<filters.length; i++) {
      Filter filter = filters[i];
      int modifiers = filter.getModifiers();
      sb.append("<filter pattern=\"").append(Util.escapeXML(filter.getNamePattern()))
      .append("\" class=\"").append((modifiers & Modifiers.CLASS) != 0).append("\" interface=\"").append((modifiers & Modifiers.INTERFACE) != 0)
      .append("\" regularExpression=\"").append(filter.isRegularExpression()).append("\"/>");
    }
    sb.append("</filters>");
    sb.append("<components>");
    ClassComponent[] classComponents = classPane.getClassComponents();
    for(int i=0; i<classComponents.length; i++) {
      ClassComponent classComponent = classComponents[i];
      String className = classComponent.getClassInfo().getClassName();
      Dimension size = classComponent.getSize();
      sb.append("<component class=\"").append(Util.escapeXML(className))
      .append("\" width=\"").append(size.width).append("\" height=\"").append(size.height);
      Dimension expandedPreferredSize = classComponent.getExpandedPreferredSize();
      if(expandedPreferredSize != null) {
        sb.append("\" expandedPreferredWidth=\"").append(expandedPreferredSize.width).append("\" expandedPreferredHeight=\"").append(expandedPreferredSize.height);
      }
      sb.append("\" expanded=\"").append(classComponent.isExpanded())
      .append("\" superTypes=\"").append(classComponent.areSuperTypesVisible()).append("\" subTypes=\"").append(classComponent.areSubTypesVisible()).append("\" compositions=\"").append(classComponent.areCompositionsVisible()).append("\" associations=\"").append(classComponent.areAssociationsVisible()).append("\"/>");
    }
    sb.append("</components>");
    sb.append("<classLocations>");
    String[] classNames = classPane.getLocatedClasses();
    for(int i=0; i<classNames.length; i++) {
      String className = classNames[i];
      Point location = classPane.getClassLocation(className);
      sb.append("<classLocation name=\"").append(Util.escapeXML(className)).append("\" x=\"").append(location.x).append("\" y=\"").append(location.y).append("\"/>");
    }
    sb.append("</classLocations>");
    sb.append("</desktop>");
    sb.append("</workspace>");
    return sb.toString();
  }

  public static void clearWorkspace(ClassPane classPane) {
    ModelDescriptor.clearModel();
    setXMLDescription(classPane, "<workspace><desktop><filters></filters><components></components></desktop></workspace>");
  }

  public static void setXMLDescription(ClassPane classPane, String description) {
    classPane.clearClassLocations();
    classPane.removeClassComponents(classPane.getClassComponents());
    try {
      DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
      DocumentBuilder builder = factory.newDocumentBuilder();
      Document document = builder.parse(new ByteArrayInputStream(description.getBytes("UTF-8")));
      NodeList nodeList = document.getChildNodes().item(0).getChildNodes();
      for(int i=0; i<nodeList.getLength(); i++) {
        Node node = nodeList.item(i);
        String name = node.getNodeName();
        if("model".equals(name)) {
          ByteArrayOutputStream out = new ByteArrayOutputStream();
          Source source = new DOMSource(node);
          Result result = new StreamResult(out);
          Transformer transformer = TransformerFactory.newInstance().newTransformer();
          transformer.transform(source, result);
          ModelDescriptor.setXMLDescription(new String(out.toByteArray(), "UTF-8"));
        }
      }
      for(int i=0; i<nodeList.getLength(); i++) {
        Node node = nodeList.item(i);
        String name = node.getNodeName();
        if("desktop".equals(name)) {
          loadDesktop(classPane, node.getChildNodes());
        }
      }
    } catch(Exception e) {
      e.printStackTrace();
    }
    classPane.adjustBounds();
    classPane.revalidate();
    classPane.repaint();
  }

  protected static void loadDesktop(ClassPane classPane, NodeList nodeList) {
    for(int i=0; i<nodeList.getLength(); i++) {
      Node node = nodeList.item(i);
      String name = node.getNodeName();
      if("filters".equals(name)) {
        Node fieldVisibilityNode = node.getAttributes().getNamedItem("fieldVisibility");
        if(fieldVisibilityNode != null) {
          classPane.setFieldVisibility(getVisibility(fieldVisibilityNode.getNodeValue()));
        } else {
          classPane.setFieldVisibility(getVisibility(null));
        }
        Node methodVisibilityNode = node.getAttributes().getNamedItem("methodVisibility");
        if(methodVisibilityNode != null) {
          classPane.setMethodVisibility(getVisibility(methodVisibilityNode.getNodeValue()));
        } else {
          classPane.setMethodVisibility(getVisibility(null));
        }
        loadFilters(classPane, node.getChildNodes());
      } else if("components".equals(name)) {
        loadComponents(classPane, node.getChildNodes());
      } else if("classLocations".equals(name)) {
        loadClassLocations(classPane, node.getChildNodes());
      } else if("selection".equals(name)) {
        ClassProcessor classProcessor = ClassProcessorRegistry.getClassProcessor(node.getAttributes().getNamedItem("classProcessor").getNodeValue());
        if(classProcessor != null) {
          classPane.setLastClassNames(node.getAttributes().getNamedItem("classes").getNodeValue());
          classPane.setLastClassProcessor(classProcessor);
        }
      }
    }
  }

  protected static void loadFilters(ClassPane classPane, NodeList nodeList) {
    List filterList = new ArrayList();
    for(int i=0; i<nodeList.getLength(); i++) {
      Node node = nodeList.item(i);
      String name = node.getNodeName();
      if("filter".equals(name)) {
        String pattern = node.getAttributes().getNamedItem("pattern").getNodeValue();
        String isClass = node.getAttributes().getNamedItem("class").getNodeValue();
        String isInterface = node.getAttributes().getNamedItem("interface").getNodeValue();
        String isRegular = node.getAttributes().getNamedItem("regularExpression").getNodeValue();
        int modifiers = 0;
        if("true".equals(isClass)) {
          modifiers |= Modifiers.CLASS;
        }
        if("true".equals(isInterface)) {
          modifiers |= Modifiers.INTERFACE;
        }
        filterList.add(new Filter(pattern, "true".equals(isRegular), modifiers));
      }
    }
    classPane.setFilters((Filter[])filterList.toArray(new Filter[0]));
  }
  
  protected static void loadComponents(ClassPane classPane, NodeList nodeList) {
    List relationRunnableList = new ArrayList();
    for(int i=0; i<nodeList.getLength(); i++) {
      Node node = nodeList.item(i);
      String name = node.getNodeName();
      if("component".equals(name)) {
        NamedNodeMap attributes = node.getAttributes();
        String className = attributes.getNamedItem("class").getNodeValue();
        String width = attributes.getNamedItem("width").getNodeValue();
        String height = attributes.getNamedItem("height").getNodeValue();
        Node expandedPreferredWidthNode = attributes.getNamedItem("expandedPreferredWidth");
        Node expandedPreferredHeightNode = attributes.getNamedItem("expandedPreferredHeight");
        Dimension expandedPreferredSize;
        if(expandedPreferredWidthNode != null && expandedPreferredHeightNode != null) {
          expandedPreferredSize = new Dimension(Integer.parseInt(expandedPreferredWidthNode.getNodeValue()), Integer.parseInt(expandedPreferredHeightNode.getNodeValue()));
        } else {
          expandedPreferredSize = null;
        }
        final String isExpanded = attributes.getNamedItem("expanded").getNodeValue();
        final String areSuperTypesVisible = attributes.getNamedItem("superTypes").getNodeValue();
        final String areSubTypesVisible = attributes.getNamedItem("subTypes").getNodeValue();
        final String areCompositionsVisible = attributes.getNamedItem("compositions").getNodeValue();
        final String areAssociationsVisible = attributes.getNamedItem("associations").getNodeValue();
        final ClassComponent classComponent = new ClassComponent(classPane, ClassInfoLoader.getClassInfo(className));
        classPane.addClassComponent(classComponent);
        classComponent.setExpandedPreferredSize(expandedPreferredSize);
        classComponent.setSize(Integer.parseInt(width), Integer.parseInt(height));
        relationRunnableList.add(new Runnable() {
          public void run() {
            if("true".equals(isExpanded)) {
              classComponent.setExpanded(true);
            }
            if("true".equals(areSuperTypesVisible)) {
              classComponent.setRelationsVisible(true, Relation.SUPER_TYPE);
            }
            if("true".equals(areSubTypesVisible)) {
              classComponent.setRelationsVisible(true, Relation.SUB_TYPE);
            }
            if("true".equals(areCompositionsVisible)) {
              classComponent.setRelationsVisible(true, Relation.COMPOSITION);
            }
            if("true".equals(areAssociationsVisible)) {
              classComponent.setRelationsVisible(true, Relation.ASSOCIATION);
            }
          }
        });
      }
    }
    for(int i=0; i<relationRunnableList.size(); i++) {
      ((Runnable)relationRunnableList.get(i)).run();
    }
  }

  protected static void loadClassLocations(ClassPane classPane, NodeList nodeList) {
    List classNameList = new ArrayList();
    List locationList = new ArrayList();
    for(int i=0; i<nodeList.getLength(); i++) {
      Node node = nodeList.item(i);
      String name = node.getNodeName();
      if("classLocation".equals(name)) {
        NamedNodeMap attributes = node.getAttributes();
        String className = attributes.getNamedItem("name").getNodeValue();
        String x = attributes.getNamedItem("x").getNodeValue();
        String y = attributes.getNamedItem("y").getNodeValue();
        Point location = new Point(Integer.parseInt(x), Integer.parseInt(y));
        classNameList.add(className);
        locationList.add(location);
        ClassComponent classComponent = classPane.getClassComponent(className);
        if(classComponent != null) {
          classComponent.setLocation(location);
        }
      }
    }
    classPane.setClassLocations((String[])classNameList.toArray(new String[0]), (Point[])locationList.toArray(new Point[0]));
    classPane.revalidate();
    classPane.repaint();
  }

}
