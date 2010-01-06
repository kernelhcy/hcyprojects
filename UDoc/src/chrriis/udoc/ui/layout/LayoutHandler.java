/*
 * Christopher Deckers (chrriis@nextencia.net)
 * http://www.nextencia.net
 * 
 * See the file "readme.txt" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
package chrriis.udoc.ui.layout;

import java.awt.Dimension;
import java.awt.Rectangle;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;

import chrriis.udoc.model.ClassInfo;
import chrriis.udoc.ui.ClassComponent;
import chrriis.udoc.ui.ClassPane;

public class LayoutHandler {

  protected static final double MAX_NODE_MOVEMENT = 200.0;
  protected static final double MIN_DIAGRAM_SIZE = 1.0;
  protected static final double MAX_REPULSIVE_FORCE_DISTANCE = 1000.0;
  protected static final double K = 1000.0;
  protected static final double C = 100.0;
  protected static final int SPRING_EMBEDDER_ITERATIONS = 1000;
  protected static final double SUPER_TYPE_WEIGHT = 2.0;
  protected static final double SUB_TYPE_WEIGHT = 1.0;
  protected static final double COMPOSITION_WEIGHT = 1.5;
  protected static final double ASSOCIATION_WEIGHT = 1.0;

  protected LayoutHandler() {}
  
  public static void adjust(ClassPane classPane) {
    new LayoutHandler().adjustLayout(classPane);
  }
  
  protected void addEdges(ClassPane classPane, Rectangle areaBounds, Node sourceNode, ClassInfo[] classInfos) {
    for(int j = 0; j < classInfos.length; j++) {
      ClassComponent classComponent = classPane.getClassComponent(classInfos[j]);
      if(classComponent != null && classComponent.isVisible()) {
        double x = (double)(classComponent.getX() - areaBounds.x) / (areaBounds.x - areaBounds.width) * 2;
        double y = (double)(classComponent.getY() - areaBounds.y) / (areaBounds.y - areaBounds.height) * 2;
        Node targetNode = new Node(classComponent, x, y);
        addEdge(sourceNode, targetNode, SUPER_TYPE_WEIGHT);
      }
    }
  }
  
  protected void adjustLayout(final ClassPane classPane) {
    ClassComponent[] classComponents = classPane.getClassComponents();
    for(int i = 0; i < classComponents.length; i++) {
      ClassComponent classComponent = classComponents[i];
      if(classComponent.isVisible()) {
        ClassInfo classInfo = classComponent.getClassInfo();
        Rectangle areaBounds = classPane.getClassComponentAreaBounds();
        double x = (double)(classComponent.getX() - areaBounds.x) / (areaBounds.x - areaBounds.width) * 2;
        double y = (double)(classComponent.getY() - areaBounds.y) / (areaBounds.y - areaBounds.height) * 2;
        Node sourceNode = new Node(classComponent, x, y);
        addNode(sourceNode);
        addEdges(classPane, areaBounds, sourceNode, classInfo.getSuperTypes());
        addEdges(classPane, areaBounds, sourceNode, classInfo.getSubTypes());
        addEdges(classPane, areaBounds, sourceNode, classInfo.getCompositions());
        addEdges(classPane, areaBounds, sourceNode, classInfo.getAssociations());
      }
    }
    computeLayout(SPRING_EMBEDDER_ITERATIONS);
    adjustLocations(classPane, classComponents);
  }

  protected void adjustLocations(ClassPane classPane, ClassComponent[] classComponents) {
    double minX = Double.POSITIVE_INFINITY;
    double maxX = Double.NEGATIVE_INFINITY;
    double minY = Double.POSITIVE_INFINITY;
    double maxY = Double.NEGATIVE_INFINITY;
    double maxWeight = 0;
    Set nodes = getLinkedNodeSet();
    for(Iterator it = nodes.iterator(); it.hasNext(); ) {
      Node node = (Node) it.next();
      if(node.getX() > maxX) {
        maxX = node.getX();
      }
      if(node.getX() < minX) {
        minX = node.getX();
      }
      if(node.getY() > maxY) {
        maxY = node.getY();
      }
      if(node.getY() < minY) {
        minY = node.getY();
      }
    }
    double minSize = MIN_DIAGRAM_SIZE;
    if(maxX - minX < minSize) {
      double midX = (maxX + minX) / 2;
      minX = midX - minSize / 2;
      maxX = midX + minSize / 2;
    }
    if(maxY - minY < minSize) {
      double midY = (maxY + minY) / 2;
      minY = midY - minSize / 2;
      maxY = midY + minSize / 2;
    }
    for(Iterator it = edgeMap.keySet().iterator(); it.hasNext(); ) {
      Edge edge = (Edge) it.next();
      double weight = edge.getWeight();
      if(weight > maxWeight) {
        maxWeight = weight;
      }
    }
    int width = (int)Math.round(Math.sqrt(classComponents.length) * 170);
    int height = width;
    double xyRatio = (maxX - minX) / (maxY - minY) / (width / height);
    if(xyRatio > 1) {
      double dy = maxY - minY;
      dy = dy * xyRatio - dy;
      minY = minY - dy / 2;
      maxY = maxY + dy / 2;
    } else if(xyRatio < 1) {
      double dx = maxX - minX;
      dx = dx / xyRatio - dx;
      minX = minX - dx / 2;
      maxX = maxX + dx / 2;
    }
    Dimension classPaneSize = classPane.getSize();
    int xOffset = (width - classPaneSize.width) / 2 + 50; // We need to consider the width of the component, so we add 50...
    height = (int)Math.round(Math.sqrt(classComponents.length) * 3 / 4 * 170);
    int yOffset = (height - classPaneSize.height) / 2 + 25; // We need to consider the height of the component, so we add 25...
    for(int i = 0; i < classComponents.length; i++) {
      ClassComponent classComponent = classComponents[i];
      if(classComponent.isVisible()) {
        Node node = (Node)nodeMap.get(new Node(classComponent, 0, 0));
        int x1 = (int)(width * (node.getX() - minX) / (maxX - minX));
        int y1 = (int)(height * (node.getY() - minY) / (maxY - minY));
        classComponent.setLocation(x1 - xOffset, y1 - yOffset);
      }
    }
    classPane.adjustBounds();
    classPane.revalidate();
    classPane.repaint();
  }
  
  protected Set getLinkedNodeSet() {
    Set linkedNodeSet = new HashSet();
    for(Iterator it = edgeMap.keySet().iterator(); it.hasNext(); ) {
      Edge edge = (Edge) it.next();
      linkedNodeSet.add(edge.getSourceNode());
      linkedNodeSet.add(edge.getTargetNode());
    }
    return linkedNodeSet;
  }

  protected void computeLayout(int iterations) {
    Node[] nodes = (Node[]) getLinkedNodeSet().toArray(new Node[0]);
    Edge[] edges = (Edge[]) edgeMap.keySet().toArray(new Edge[0]);
    double maxRepulsiveForceDistance = MAX_REPULSIVE_FORCE_DISTANCE;
    for(int it = 0; it < iterations; it++) {
      for(int i = 0; i < nodes.length; i++) {
        for(int j = i + 1; j < nodes.length; j++) {
          Node node1 = nodes[i];
          Node node2 = nodes[j];
          double deltaX = node2.getX() - node1.getX();
          double deltaY = node2.getY() - node1.getY();
          double distanceSquared = deltaX * deltaX + deltaY * deltaY;
          if(distanceSquared < 0.01) {
            deltaX = Math.random() / 10 + 0.1;
            deltaY = Math.random() / 10 + 0.1;
            distanceSquared = deltaX * deltaX + deltaY * deltaY;
          }
          double distance = Math.sqrt(distanceSquared);
          if(distance < maxRepulsiveForceDistance) {
            double repulsiveForce = K * K / distance;
            node1.setFX(node1.getFX() - repulsiveForce * deltaX / distance);
            node1.setFY(node1.getFY() - repulsiveForce * deltaY / distance);
            node2.setFX(node2.getFX() + repulsiveForce * deltaX / distance);
            node2.setFY(node2.getFY() + repulsiveForce * deltaY / distance);
          }
        }
      }
      for(int i = 0; i < edges.length; i++) {
        Edge edge = edges[i];
        Node sourceNode = edge.getSourceNode();
        Node targetNode = edge.getTargetNode();
        double deltaX = targetNode.getX() - sourceNode.getX();
        double deltaY = targetNode.getY() - sourceNode.getY();
        double distanceSquared = deltaX * deltaX + deltaY * deltaY;
        if(distanceSquared < 0.01) {
          deltaX = Math.random() / 10 + 0.1;
          deltaY = Math.random() / 10 + 0.1;
          distanceSquared = deltaX * deltaX + deltaY * deltaY;
        }
        double distance = Math.sqrt(distanceSquared);
        if(distance > maxRepulsiveForceDistance) {
          distance = maxRepulsiveForceDistance;
        }
        distanceSquared = distance * distance;
        double attractiveForce = (distanceSquared - K * K) / K;
        double weight = edge.getWeight();
        if(weight < 1) {
          weight = 1;
        }
        attractiveForce *= Math.log(weight) * 0.5 + 1;
        targetNode.setFX(targetNode.getFX() - attractiveForce * deltaX / distance);
        targetNode.setFY(targetNode.getFY() - attractiveForce * deltaY / distance);
        sourceNode.setFX(sourceNode.getFX() + attractiveForce * deltaX / distance);
        sourceNode.setFY(sourceNode.getFY() + attractiveForce * deltaY / distance);
      }
      for(int i = 0; i < nodes.length; i++) {
        Node node = nodes[i];
        double xMovement = C * node.getFX();
        double yMovement = C * node.getFY();
        double max = MAX_NODE_MOVEMENT;
        if(xMovement > max) {
          xMovement = max;
        } else if(xMovement < -max) {
          xMovement = -max;
        }
        if(yMovement > max) {
          yMovement = max;
        } else if(yMovement < -max) {
          yMovement = -max;
        }
        node.setX(node.getX() + xMovement);
        node.setY(node.getY() + yMovement);
        node.setFX(0);
        node.setFY(0);
      }
    }
  }

  protected Map nodeMap = new HashMap();

  protected void addNode(Node node) {
    if(nodeMap.containsKey(node)) {
      node = (Node) nodeMap.get(node);
    } else {
      nodeMap.put(node, node);
    }
    node.setWeight(node.getWeight() + 1);
  }

  protected Map edgeMap = new HashMap();

  protected boolean addEdge(Node sourceNode, Node targetNode, double weight) {
    if(sourceNode.equals(targetNode) || weight <= 0) {
      return false;
    }
    addNode(sourceNode);
    addNode(targetNode);
    Edge edge = new Edge(sourceNode, targetNode);
    if(edgeMap.containsKey(edge)) {
      edge = (Edge) edgeMap.get(edge);
    } else {
      sourceNode = (Node) nodeMap.get(sourceNode);
      targetNode = (Node) nodeMap.get(targetNode);
      edge = new Edge(sourceNode, targetNode);
      edgeMap.put(edge, edge);
    }
    edge.setWeight(edge.getWeight() + weight);
    return true;
  }

}