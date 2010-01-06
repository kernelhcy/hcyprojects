/*
 * Christopher Deckers (chrriis@nextencia.net)
 * http://www.nextencia.net
 * 
 * See the file "readme.txt" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
package chrriis.udoc.ui.layout;

public class Edge {

  private Node sourceNode;
  private Node targetNode;
  
  public Edge(Node sourceNode, Node targetNode) {
    this.sourceNode = sourceNode;
    this.targetNode = targetNode;
  }

  private double weight;

  public void setWeight(double weight) {
    this.weight = weight;
  }

  public double getWeight() {
    return weight;
  }

  public Node getSourceNode() {
    return sourceNode;
  }

  public Node getTargetNode() {
    return targetNode;
  }

  public int hashCode() {
    return sourceNode.hashCode() + targetNode.hashCode();
  }
  
  public boolean equals(Object o) {
    if (o instanceof Edge) {
      Edge edge = (Edge) o;
      return sourceNode.equals(edge.sourceNode) && targetNode.equals(edge.targetNode) || sourceNode.equals(edge.targetNode) && targetNode.equals(edge.sourceNode);
    }
    return false;
  }

}