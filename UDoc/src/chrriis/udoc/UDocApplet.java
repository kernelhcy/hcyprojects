/*
 * Christopher Deckers (chrriis@nextencia.net)
 * http://www.nextencia.net
 * 
 * See the file "readme.txt" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
package chrriis.udoc;

import java.awt.BorderLayout;
import java.awt.Container;
import java.io.BufferedInputStream;
import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.net.URL;
import java.util.zip.InflaterInputStream;

import javax.swing.JApplet;
import javax.swing.SwingUtilities;

import chrriis.udoc.model.URLConnectionEx;
import chrriis.udoc.model.Util;
import chrriis.udoc.model.processor.ClassProcessor;
import chrriis.udoc.model.processor.ClassProcessorRegistry;
import chrriis.udoc.model.processor.binary.BinaryClassProcessor;
import chrriis.udoc.model.processor.javadoc.JavadocClassProcessor;
import chrriis.udoc.model.processor.source.SourceClassProcessor;
import chrriis.udoc.ui.ClassPane;
import chrriis.udoc.ui.Workspace;

public class UDocApplet extends JApplet {

  static {
    ClassProcessorRegistry.register(new JavadocClassProcessor());
    ClassProcessorRegistry.register(new BinaryClassProcessor());
    ClassProcessorRegistry.register(new SourceClassProcessor());
  }

  protected ClassPane classPane;
  
  protected String getURLString(String resource) {
    if(resource == null) {
      return null;
    }
    if(resource.indexOf(":/") == -1) {
      if(resource.startsWith("/")) {
        URL documentBase = getDocumentBase();
        resource = documentBase.getProtocol() + "://" + documentBase.getHost() + '/' + resource;
      } else {
        String directory = getDocumentBase().toExternalForm();
        resource = directory.substring(0, directory.lastIndexOf('/') + 1) + resource;
      }
    }
    return resource;
  }
  
  public void init() {
    classPane = new ClassPane();
    classPane.setTitle("UDoc");
    Container contentPane = getContentPane();
    contentPane.add(classPane, BorderLayout.CENTER);
    setSize(800, 600);
    contentPane.invalidate();
    doLayout();
    String workspace = getURLString(getParameter("workspace"));
    if(workspace != null) {
      classPane.setLocked(true);
      try {
        InputStream in = new BufferedInputStream(new InflaterInputStream(URLConnectionEx.openConnection(new URL(workspace)).getInputStream()));
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        byte[] bytes = new byte[1024];
        for(int count; (count=in.read(bytes)) >= 0; ) {
          out.write(bytes, 0, count);
        }
        Workspace.setXMLDescription(classPane, new String(out.toByteArray(), "UTF-8"));
        in.close();
      } catch(Exception ex) {
        try {
          InputStream in = new BufferedInputStream(new URL(workspace).openConnection().getInputStream());
          ByteArrayOutputStream out = new ByteArrayOutputStream();
          byte[] bytes = new byte[1024];
          for(int count; (count=in.read(bytes)) >= 0; ) {
            out.write(bytes, 0, count);
          }
          Workspace.setXMLDescription(classPane, new String(out.toByteArray(), "UTF-8"));
          in.close();
        } catch(Exception ex2) {
          ex2.printStackTrace();
        }
      }
    } else {
      String processorID = getParameter("processorID");
      final String classNames = getParameter("classNames");
      Util.setPathSeparator(getParameter("pathSeparator"));
      final ClassProcessor classProcessor = ClassProcessorRegistry.getClassProcessor(processorID);
      if(classProcessor != null) {
        if(classProcessor instanceof JavadocClassProcessor) {
          String documentation = getURLString(getParameter("documentationRoot"));
          if(documentation != null) {
            ((JavadocClassProcessor)classProcessor).setDocumentationRoot(documentation);
          }
        } else if(classProcessor instanceof BinaryClassProcessor) {
          String classpath = getURLString(getParameter("classPath"));
          if(classpath != null) {
            ((BinaryClassProcessor)classProcessor).setClassPath(classpath);
          }
        } else if(classProcessor instanceof SourceClassProcessor) {
          String sourcepath = getURLString(getParameter("sourcePath"));
          String classpath = getURLString(getParameter("classPath"));
          if(sourcepath != null && classpath != null) {
            SourceClassProcessor sourceClassProcessor = (SourceClassProcessor)classProcessor;
            sourceClassProcessor.setSourcePath(sourcepath);
            sourceClassProcessor.setClassPath(classpath);
          }
        }
        SwingUtilities.invokeLater(new Runnable() {
          public void run() {
            classPane.setContent(classNames, classProcessor);
          }
        });
      }
    }
  }
  
  public void requestFocus() {
    classPane.getClassComponentPane().requestFocus();
  }
  
  public void start() {
    transferFocus();
    requestFocus();
    if(isShowing()) {
      transferFocus();
      requestFocus();
    } else {
      SwingUtilities.invokeLater(new Runnable() {
        public void run() {
          start();
        }
      });
    }
  }

}
