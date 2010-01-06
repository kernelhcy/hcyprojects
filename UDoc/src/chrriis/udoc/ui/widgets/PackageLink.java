/*
 * Christopher Deckers (chrriis@nextencia.net)
 * http://www.nextencia.net
 * 
 * See the file "readme.txt" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
package chrriis.udoc.ui.widgets;

import chrriis.udoc.model.PackageInfo;
import chrriis.udoc.ui.ClassComponent;


public class PackageLink extends Link {

  protected ClassComponent classComponent;
  protected PackageInfo packageInfo;

  public PackageLink(ClassComponent classComponent, String text, PackageInfo packageInfo) {
    super(classComponent, text);
    this.classComponent = classComponent;
    this.packageInfo = packageInfo;
  }

  protected void processLink() {
    classComponent.openPackage(packageInfo);
  }

}

