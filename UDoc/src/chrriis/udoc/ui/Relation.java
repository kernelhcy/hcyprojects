/*
 * Christopher Deckers (chrriis@nextencia.net)
 * http://www.nextencia.net
 * 
 * See the file "readme.txt" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
package chrriis.udoc.ui;

public class Relation {
  public static final int SUPER_TYPE = 1;
  public static final int SUB_TYPE = 2;
  public static final int COMPOSITION = 3;
  public static final int ASSOCIATION = 4;
  protected int type;
  protected ClassComponent classComponent1;
  protected ClassComponent classComponent2;
  public Relation(int type, ClassComponent classComponent1, ClassComponent classComponent2) {
    this.type = type;
    this.classComponent1 = classComponent1;
    this.classComponent2 = classComponent2;
  }
  public int getType() {
    return type;
  }
  public ClassComponent getClassComponent1() {
    return classComponent1;
  }
  public ClassComponent getClassComponent2() {
    return classComponent2;
  }
  public boolean equals(Object o) {
    Relation relation = (Relation)o;
    return relation.type == type && relation.classComponent1 == classComponent1 && relation.classComponent2 == classComponent2;
  }
  public Object getKey() {
    int h1 = classComponent1.hashCode();
    int h2 = classComponent2.hashCode();
    return Math.min(h1, h2) + "|" + Math.max(h1, h2);
  }
}