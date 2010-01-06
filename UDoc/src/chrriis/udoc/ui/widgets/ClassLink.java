/*
 * Christopher Deckers (chrriis@nextencia.net)
 * http://www.nextencia.net
 * 
 * See the file "readme.txt" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
package chrriis.udoc.ui.widgets;

import chrriis.udoc.model.ClassInfo;
import chrriis.udoc.ui.ClassComponent;

public class ClassLink extends Link {

  protected ClassComponent classComponent;
  protected ClassInfo classInfo;

  public ClassLink(ClassComponent classComponent, String text, ClassInfo classInfo) {
    super(classComponent, text);
    this.classComponent = classComponent;
    this.classInfo = classInfo;
  }

  protected void processLink() {
    classComponent.openClass(classInfo);
  }

  public ClassInfo getClassInfo() {
    return classInfo;
  }

}

