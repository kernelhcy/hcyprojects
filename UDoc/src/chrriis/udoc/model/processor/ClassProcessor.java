/*
 * Christopher Deckers (chrriis@nextencia.net)
 * http://www.nextencia.net
 * 
 * See the file "readme.txt" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
package chrriis.udoc.model.processor;

import java.io.InputStream;

import javax.swing.JComponent;
import javax.swing.JTree;

import chrriis.udoc.model.ClassInfo;
import chrriis.udoc.model.FieldInfo;
import chrriis.udoc.model.MethodInfo;
import chrriis.udoc.model.PackageInfo;

public abstract class ClassProcessor {

  public abstract String getProcessorID();

  public String getProcessorName() {
    return getProcessorID();
  }

  public String getProcessorDescription() {
    return null;
  }
  
  public JComponent getParametersComponent() {
    return null;
  }

  public abstract InputStream getClassInfoDataInputStream(String className);

  public void openPackage(PackageInfo packageInfo) {
  }

  public void openClass(ClassInfo classInfo) {
  }

  public void openMethod(MethodInfo methodInfo) {
  }

  public void openField(FieldInfo fieldInfo) {
  }

  public String getXMLDescription() {
    return null;
  }

  public void loadXMLDescription(String xmlDescription) {
  }

  public void destroyClassInfo(ClassInfo classInfo) {
  }

  public void loadClassBrowser(JTree tree) {
  }

}
