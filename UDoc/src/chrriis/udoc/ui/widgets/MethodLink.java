/*
 * Christopher Deckers (chrriis@nextencia.net)
 * http://www.nextencia.net
 * 
 * See the file "readme.txt" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
package chrriis.udoc.ui.widgets;

import chrriis.udoc.model.ClassInfo;
import chrriis.udoc.model.MethodInfo;
import chrriis.udoc.ui.ClassComponent;

public class MethodLink extends Link {

  protected ClassComponent classComponent;
  protected ClassInfo classInfo;
  protected MethodInfo methodInfo;

  public MethodLink(ClassComponent classComponent, String text, ClassInfo classInfo, MethodInfo methodInfo) {
    super(classComponent, text);
    this.classComponent = classComponent;
    this.classInfo = classInfo;
    this.methodInfo = methodInfo;
  }

  protected void processLink() {
    classComponent.openMethod(methodInfo);
  }

  public MethodInfo getMethodInfo() {
    return methodInfo;
  }

}

