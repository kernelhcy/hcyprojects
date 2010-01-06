/*
 * Christopher Deckers (chrriis@nextencia.net)
 * http://www.nextencia.net
 * 
 * See the file "readme.txt" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
package chrriis.udoc.ui.layout;

import chrriis.udoc.ui.ClassComponent;

public class Node {

  protected ClassComponent classComponent;

  protected double x;
  protected double y;
  
  public Node(ClassComponent classComponent, double x, double y) {
    this.classComponent = classComponent;
    this.x = x;
    this.y = y;
  }

  public double getX() {
    return x;
  }
  
  public void setX(double x) {
    this.x = x;
  }

  public double getY() {
    return y;
  }
  
  public void setY(double y) {
    this.y = y;
  }

  protected double fx;

  public double getFX() {
    return fx;
  }
  
  public void setFX(double fx) {
    this.fx = fx;
  }

  protected double fy;

  public double getFY() {
    return fy;
  }
  
  public void setFY(double fy) {
    this.fy = fy;
  }

  protected double weight;

  public void setWeight(double weight) {
    this.weight = weight;
  }

  public double getWeight() {
    return weight;
  }

  public int hashCode() {
    return classComponent.hashCode();
  }
  
  public boolean equals(Object o) {
    if (o instanceof Node) {
      return classComponent == ((Node) o).classComponent;
    }
    return false;
  }

}