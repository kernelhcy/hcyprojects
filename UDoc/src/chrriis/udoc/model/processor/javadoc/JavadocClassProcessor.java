/*
 * Christopher Deckers (chrriis@nextencia.net)
 * http://www.nextencia.net
 * 
 * See the file "readme.txt" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
package chrriis.udoc.model.processor.javadoc;

import java.io.BufferedReader;
import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.URL;

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

import chrriis.udoc.model.ClassInfo;
import chrriis.udoc.model.ClassInfoLoader;
import chrriis.udoc.model.FieldInfo;
import chrriis.udoc.model.MethodInfo;
import chrriis.udoc.model.PackageInfo;
import chrriis.udoc.model.URLConnectionEx;
import chrriis.udoc.model.Util;
import chrriis.udoc.model.processor.ClassProcessor;
import chrriis.udoc.ui.webpage.WebpageViewer;

public class JavadocClassProcessor extends ClassProcessor {

  public String getProcessorID() {
    return "JavadocClassProcessor";
  }

  public String getProcessorName() {
    return "Javadoc";
  }
  
  public String getProcessorDescription() {
    return "Load class diagrams from a Java documentation";
  }
  
  protected String documentationRoot;

  public JComponent getParametersComponent() {
    return new JavadocParametersComponent(this);
  }

  public InputStream getClassInfoDataInputStream(String className) {
    try {
      int lastDotIndex = className.lastIndexOf('.');
      String packageName = lastDotIndex == -1? "": className.substring(0, lastDotIndex);
      String documentationRoot = JavadocClassInfoLoader.getPackageDocumentationRoot(packageName);
      if(documentationRoot == null) {
        documentationRoot = getDocumentationRootURI();
      }
      String classData = JavadocClassInfoLoader.loadClassData(className, documentationRoot);
      if(classData == null) {
        return null;
      }
      return new ByteArrayInputStream(classData.getBytes("UTF-8"));
    } catch(Exception e) {
      e.printStackTrace();
    }
    return null;
  }

  public String getDocumentationRoot() {
    return documentationRoot;
  }

  protected String getDocumentationRootURI() {
    if(documentationRoot.indexOf(":/") < 0) {
      return new File(documentationRoot).toURI().toString();
    }
    return documentationRoot;
  }
  
  public void setDocumentationRoot(String documentationRoot) {
    this.documentationRoot = documentationRoot;
    javadocNode = null;
  }

  protected String getLinkRoot(PackageInfo packageInfo) {
    String packageName = packageInfo.getName();
    String link = JavadocClassInfoLoader.getPackageDocumentationRoot(packageName);
    if(link == null) {
      link = getDocumentationRootURI() + '/' + packageName.replace('.', '/');
    }
    return link;
  }

  protected String getLink(PackageInfo packageInfo) {
    return getLinkRoot(packageInfo) + "/package-summary.html";
  }
  
  protected String getLink(ClassInfo classInfo) {
    String packagePath = classInfo.getPackage().getName();
    if(packagePath.length() > 0) {
      packagePath = packagePath.replace('.', '/') + "/";
    }
    String docPath = packagePath + Util.unescapeClassName(classInfo.getName()) + ".html";
    String linkRoot = getLinkRoot(classInfo.getPackage());
    if(linkRoot != null) {
      docPath = linkRoot + "/" + docPath;
    }
    return docPath;
  }

  protected String getLink(MethodInfo methodInfo) {
    StringBuffer sb = new StringBuffer();
    FieldInfo[] parameters = methodInfo.getParameters();
    for(int i=0; i<parameters.length; i++) {
      if(i > 0) {
        sb.append(", ");
      }
      String classDeclaration = parameters[i].getClassDeclaration();
      int count = 0;
      for(int j=0; j<classDeclaration.length(); j++) {
        char c = classDeclaration.charAt(j);
        switch(c) {
          case '<': count++; break;
          case '>': count--; break;
          default:
            if(count == 0) {
              sb.append(c);
            }
            break;
        }
      }
    }
    return getLink(methodInfo.getClassInfo()) + "#" + methodInfo.getName() + "(" + sb.toString() + ")";
  }

  protected String getLink(FieldInfo fieldInfo) {
    return getLink(fieldInfo.getClassInfo()) + "#" + fieldInfo.getName();
  }

  public void openPackage(PackageInfo packageInfo) {
    WebpageViewer.open(packageInfo.getName(), getLink(packageInfo));
  }

  public void openClass(ClassInfo classInfo) {
    WebpageViewer.open(classInfo.getClassName(), getLink(classInfo));
  }

  public void openMethod(MethodInfo methodInfo) {
    WebpageViewer.open(methodInfo.getClassInfo().getClassName(), getLink(methodInfo));
  }

  public void openField(FieldInfo fieldInfo) {
    WebpageViewer.open(fieldInfo.getClassInfo().getClassName(), getLink(fieldInfo));
  }

  public String getXMLDescription() {
    StringBuffer sb = new StringBuffer();
    sb.append("<javadocClassProcessorParameter>");
    sb.append("<documentation value=\"").append(documentationRoot == null? "": Util.escapeXML(documentationRoot)).append("\"/>");
    sb.append("<packages>");
    String[] packageNames = JavadocClassInfoLoader.getPackageNames();
    for (int i = 0; i < packageNames.length; i++) {
      String packageName = packageNames[i];
      sb.append("<package name=\"").append(Util.escapeXML(packageName)).append("\" documentation=\"").append(Util.escapeXML(JavadocClassInfoLoader.getPackageDocumentationRoot(packageName))).append("\"/>");
    }
    sb.append("</packages>");
    sb.append("</javadocClassProcessorParameter>");
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
        if("documentation".equals(name)) {
          setDocumentationRoot(node.getAttributes().getNamedItem("value").getNodeValue());
        } else if("packages".equals(name)) {
          loadPackages(node.getChildNodes());
        }
      }
    } catch(Exception e) {
      e.printStackTrace();
    }
  }

  protected void loadPackages(NodeList nodeList) {
    JavadocClassInfoLoader.clearPackageDocumentationRootInformation();
    for(int i=0; i<nodeList.getLength(); i++) {
      Node node = nodeList.item(i);
      String name = node.getNodeName();
      if("package".equals(name)) {
        JavadocClassInfoLoader.setPackageDocumentationRoot(node.getAttributes().getNamedItem("name").getNodeValue(), node.getAttributes().getNamedItem("documentation").getNodeValue());
      }
    }
  }

  public void destroyClassInfo(ClassInfo classInfo) {
    String packageName = classInfo.getPackage().getName();
    ClassInfo[] classInfos = ClassInfoLoader.getClassInfos();
    for(int i=0; i<classInfos.length; i++) {
      ClassInfo cInfo = classInfos[i];
      if(cInfo.getClassProcessor() == this && cInfo.getPackage().getName().equals(packageName)) {
        return;
      }
    }
    JavadocClassInfoLoader.removePackageDocumentationRoot(packageName);
  }
  
  protected DefaultMutableTreeNode javadocNode;
  
  public void loadClassBrowser(final JTree tree) {
    if(javadocNode != null) {
      tree.setModel(new DefaultTreeModel(javadocNode));
      return;
    }
    final String documentationRoot = this.documentationRoot;
    final DefaultMutableTreeNode node = (DefaultMutableTreeNode)tree.getModel().getRoot();
    node.removeAllChildren();
    new Thread(){
      public void run() {
        String path = getDocumentationRootURI() + "/allclasses-frame.html";
        try {
          BufferedReader reader = new BufferedReader(new InputStreamReader(URLConnectionEx.openConnection(new URL(path)).getInputStream()));
          for(String line; (line = reader.readLine()) != null; ) {
            line = line.trim();
            if(line.startsWith("<A HREF=\"")) {
              line = line.substring("<A HREF=\"".length());
              line = line.substring(0, line.indexOf('"') - ".html".length());
              line = line.replace("$", "$$").replace('.', '$').replace('/', '.');
              addTreeClass(line, node);
            }
          }
        } catch(Exception e) {
          e.printStackTrace();
        }
        SwingUtilities.invokeLater(new Runnable() {
          public void run() {
            if(documentationRoot == null? JavadocClassProcessor.this.documentationRoot != null: !documentationRoot.equals(JavadocClassProcessor.this.documentationRoot)) {
              return;
            }
            javadocNode = node;
            tree.setModel(new DefaultTreeModel(javadocNode));
          }
        });
      }
    }.start();
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
