/*
 * Christopher Deckers (chrriis@nextencia.net)
 * http://www.nextencia.net
 * 
 * See the file "readme.txt" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
package chrriis.udoc.model;

public class PackageInfo {

  protected String packageName;

  protected PackageInfo(String packageName, String documentationRoot) {
    this.packageName = packageName;
  }

  public String getName() {
    return packageName;
  }

}
