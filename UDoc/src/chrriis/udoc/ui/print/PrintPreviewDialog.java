/*
 * Christopher Deckers (chrriis@nextencia.net)
 * http://www.nextencia.net
 * 
 * See the file "readme.txt" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
package chrriis.udoc.ui.print;

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Dialog;
import java.awt.Dimension;
import java.awt.FlowLayout;
import java.awt.Frame;
import java.awt.Graphics;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.GridLayout;
import java.awt.Image;
import java.awt.Point;
import java.awt.Rectangle;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.ItemEvent;
import java.awt.event.ItemListener;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.awt.image.BufferedImage;

import javax.swing.BorderFactory;
import javax.swing.Box;
import javax.swing.BoxLayout;
import javax.swing.ButtonGroup;
import javax.swing.ImageIcon;
import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JDialog;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JRadioButton;
import javax.swing.JTextField;
import javax.swing.border.TitledBorder;
import javax.swing.event.DocumentEvent;
import javax.swing.event.DocumentListener;

import chrriis.udoc.ui.ClassPane;
import chrriis.udoc.ui.widgets.ArrowButton;

public class PrintPreviewDialog extends JDialog {

  protected static final String TITLE = "Print Preview";

  protected static final int PREVIEW_SIZE = 300;
  
  protected static final String PORTRAIT = "Portrait";
  protected static final String LANDSCAPE = "Landscape";

  protected int x;
  protected int y;

  protected JLabel label;
  protected JPanel labelPanel;
  protected JComboBox orientationComboBox;
  protected JButton leftButton;
  protected JButton rightButton;
  protected JButton upButton;
  protected JButton downButton;
  protected JRadioButton scaleRadioButton;
  protected JTextField scaleField;
  protected JTextField pageFitField;
  protected JTextField hFitField;
  protected JTextField vFitField;
  protected JRadioButton pageFitRadioButton;
  protected JRadioButton hFitRadioButton;
  protected JRadioButton vFitRadioButton;
  protected JLabel statusLabel;

  public PrintPreviewDialog(Dialog dialog, ClassPane classPane) {
    super(dialog, TITLE);
    init(classPane);
  }
  
  public PrintPreviewDialog(Frame frame, ClassPane classPane) {
    super(frame, TITLE);
    init(classPane);
  }

  protected void init(final ClassPane classPane) {
    addWindowListener(new WindowAdapter() {
      public void windowClosing(WindowEvent e) {
        if(image != null) {
          image.flush();
        }
      }
    });
    JPanel contentPane = new JPanel(new BorderLayout(0, 0));
    contentPane.setBorder(BorderFactory.createEmptyBorder(2, 2, 2, 2));
    label = new JLabel();
    label.setBorder(null);
    label.setHorizontalAlignment(JLabel.CENTER);
    label.setVerticalAlignment(JLabel.CENTER);
    label.setPreferredSize(new Dimension(PREVIEW_SIZE, PREVIEW_SIZE));
    labelPanel = new JPanel(new BorderLayout(0, 0));
    labelPanel.setBorder(BorderFactory.createEtchedBorder());
    labelPanel.add(label, BorderLayout.CENTER);
    GridBagLayout gridBag2 = new GridBagLayout();
    JPanel labelContainer = new JPanel(gridBag2) {
      public Dimension getPreferredSize() {
        Dimension size = super.getPreferredSize();
        int max = Math.max(size.width, size.height);
        return new Dimension(max, max);
      }
    };
    labelContainer.add(labelPanel);
    JPanel centerPanel = new JPanel(new BorderLayout(0, 0));
    centerPanel.setBorder(BorderFactory.createEmptyBorder(2, 0, 2, 0));
    GridBagLayout gridBag = new GridBagLayout();
    JPanel previewPanel = new JPanel(gridBag);
    TitledBorder previewTitleBorder = BorderFactory.createTitledBorder("Preview");
    previewPanel.setBorder(previewTitleBorder);
    GridBagConstraints cons = new GridBagConstraints();
    cons.gridx = 0;
    cons.gridy = 0;
    cons.fill = GridBagConstraints.BOTH;
    cons.weightx = 1.0;
    cons.weighty = 1.0;
    gridBag.setConstraints(labelContainer, cons);
    previewPanel.add(labelContainer);
    cons.fill = GridBagConstraints.NONE;
    cons.weightx = 0;
    cons.weighty = 0;
    cons.gridx++;
    JPanel verticalButtonsPanel = new JPanel(new GridLayout(0, 1, 2, 0));
    upButton = new ArrowButton(ArrowButton.NORTH);
    upButton.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        if(y == 0) {
          return;
        }
        y--;
        adjust(classPane);
      }
    });
    verticalButtonsPanel.add(upButton);
    downButton = new ArrowButton(ArrowButton.SOUTH);
    downButton.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        if(y == pageCount.y - 1) {
          return;
        }
        y++;
        adjust(classPane);
      }
    });
    verticalButtonsPanel.add(downButton);
    gridBag.setConstraints(verticalButtonsPanel, cons);
    previewPanel.add(verticalButtonsPanel);
    cons.gridx = 0;
    cons.gridy++;
    JPanel horizontalButtonsPanel = new JPanel(new GridLayout(1, 0, 0, 2));
    leftButton = new ArrowButton(ArrowButton.WEST);
    leftButton.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        if(x == 0) {
          return;
        }
        x--;
        adjust(classPane);
      }
    });
    horizontalButtonsPanel.add(leftButton);
    rightButton = new ArrowButton(ArrowButton.EAST);
    rightButton.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        if(x == pageCount.x - 1) {
          return;
        }
        x++;
        adjust(classPane);
      }
    });
    horizontalButtonsPanel.add(rightButton);
    gridBag.setConstraints(horizontalButtonsPanel, cons);
    previewPanel.add(horizontalButtonsPanel);
    cons.gridx = 0;
    cons.gridy++;
    cons.gridwidth = 2;
    cons.anchor = GridBagConstraints.WEST;
    statusLabel = new JLabel(" ");
    gridBag.setConstraints(statusLabel, cons);
    previewPanel.add(statusLabel);
    centerPanel.add(previewPanel, BorderLayout.CENTER);
    contentPane.add(centerPanel, BorderLayout.CENTER);
    JPanel settingsPanel = new JPanel();
    BoxLayout layout = new BoxLayout(settingsPanel, BoxLayout.Y_AXIS);
    settingsPanel.setLayout(layout);
    settingsPanel.add(Box.createVerticalGlue());
    JPanel orientationPanel = new JPanel(new BorderLayout(0, 0)) {
      public Dimension getMaximumSize() {
        return new Dimension(super.getMaximumSize().width, getPreferredSize().height);
      }
    };
    orientationPanel.setBorder(BorderFactory.createTitledBorder("Orientation"));
    orientationComboBox = new JComboBox(new Object[] {PORTRAIT, LANDSCAPE});
    orientationComboBox.setSelectedItem(PrintProcessing.isPortrait()? PORTRAIT: LANDSCAPE);
    orientationComboBox.addItemListener(new ItemListener() {
      public void itemStateChanged(ItemEvent e) {
        if(e.getStateChange() == ItemEvent.SELECTED) {
          adjust(classPane);
        }
      }
    });
    orientationPanel.add(orientationComboBox, BorderLayout.SOUTH);
    settingsPanel.add(orientationPanel);
    JPanel scalingPanel = new JPanel();
    scalingPanel.setBorder(BorderFactory.createTitledBorder("Scale"));
    BoxLayout boxLayout = new BoxLayout(scalingPanel, BoxLayout.Y_AXIS);
    scalingPanel.setLayout(boxLayout);
//    scalingPanel.add(orientationPanel);
    ButtonGroup group = new ButtonGroup();
    JPanel scalePanel = new JPanel(new FlowLayout(FlowLayout.LEFT, 0, 0)) {
      public Dimension getMaximumSize() {
        return new Dimension(super.getMaximumSize().width, getPreferredSize().height);
      }
    };
    scaleRadioButton = new JRadioButton("Ratio ");
    group.add(scaleRadioButton);
    scalePanel.add(scaleRadioButton);
    scaleField = new JTextField(PrintProcessing.getScalePercentageString(), 3);
    scalePanel.add(scaleField);
    scalePanel.add(new JLabel("%"));
    scalingPanel.add(scalePanel);
    JPanel pageFitPanel = new JPanel(new FlowLayout(FlowLayout.LEFT, 0, 0)) {
      public Dimension getMaximumSize() {
        return new Dimension(super.getMaximumSize().width, getPreferredSize().height);
      }
    };
    pageFitRadioButton = new JRadioButton("Fit");
    group.add(pageFitRadioButton);
    pageFitPanel.add(pageFitRadioButton);
    pageFitField = new JTextField(PrintProcessing.getPageFitString(), 3);
    pageFitPanel.add(pageFitField);
    pageFitPanel.add(new JLabel(" pages"));
    scalingPanel.add(pageFitPanel);
    JPanel hFitPanel = new JPanel(new FlowLayout(FlowLayout.LEFT, 0, 0)) {
      public Dimension getMaximumSize() {
        return new Dimension(super.getMaximumSize().width, getPreferredSize().height);
      }
    };
    hFitRadioButton = new JRadioButton("Fit");
    group.add(hFitRadioButton);
    hFitPanel.add(hFitRadioButton);
    hFitField = new JTextField(PrintProcessing.getHorizontalPageFitString(), 3);
    hFitPanel.add(hFitField);
    hFitPanel.add(new JLabel(" horizontal pages"));
    scalingPanel.add(hFitPanel);
    JPanel vFitPanel = new JPanel(new FlowLayout(FlowLayout.LEFT, 0, 0)) {
      public Dimension getMaximumSize() {
        return new Dimension(super.getMaximumSize().width, getPreferredSize().height);
      }
    };
    vFitRadioButton = new JRadioButton("Fit");
    group.add(vFitRadioButton);
    vFitPanel.add(vFitRadioButton);
    vFitField = new JTextField(PrintProcessing.getVerticalPageFitString(), 3);
    vFitPanel.add(vFitField);
    vFitPanel.add(new JLabel(" vertical pages"));
    scalingPanel.add(vFitPanel);
    switch(PrintProcessing.getFitType()) {
      case PrintProcessing.PAGE_FIT:
        pageFitRadioButton.setSelected(true);
        break;
      case PrintProcessing.HORIZONTAL_PAGE_FIT:
        hFitRadioButton.setSelected(true);
        break;
      case PrintProcessing.VERTICAL_PAGE_FIT:
        vFitRadioButton.setSelected(true);
        break;
      default:
        scaleRadioButton.setSelected(true);
        break;
    }
    ItemListener itemListener = new ItemListener() {
      public void itemStateChanged(ItemEvent e) {
        if(e.getStateChange() == ItemEvent.SELECTED) {
          adjust(classPane);
        }
      }
    };
    scaleRadioButton.addItemListener(itemListener);
    pageFitRadioButton.addItemListener(itemListener);
    hFitRadioButton.addItemListener(itemListener);
    vFitRadioButton.addItemListener(itemListener);
    DocumentListener documentListener = new DocumentListener() {
      public void changedUpdate(DocumentEvent e) {
        adjust(classPane);
      }
      public void insertUpdate(DocumentEvent e) {
        adjust(classPane);
      }
      public void removeUpdate(DocumentEvent e) {
        adjust(classPane);
      }
    };
    scaleField.getDocument().addDocumentListener(documentListener);
    pageFitField.getDocument().addDocumentListener(documentListener);
    hFitField.getDocument().addDocumentListener(documentListener);
    vFitField.getDocument().addDocumentListener(documentListener);
    settingsPanel.add(scalingPanel);
    
    JPanel buttonPanel = new JPanel(new GridLayout(0, 1, 0, 2)) {
      public Dimension getMaximumSize() {
        return getPreferredSize();
      }
    };
    JButton printButton = new JButton("Print...");
    printButton.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        saveConfiguration();
        dispose();
        PrintProcessing.print(classPane);
      }
    });
    buttonPanel.add(printButton);
    JButton cancelButton = new JButton("Cancel");
    cancelButton.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        dispose();
      }
    });
    buttonPanel.add(cancelButton);
    settingsPanel.add(Box.createVerticalGlue());
    settingsPanel.add(buttonPanel);
    
    contentPane.add(settingsPanel, BorderLayout.EAST);
    getContentPane().add(contentPane, BorderLayout.CENTER);
    adjust(classPane);
  }

  protected void saveConfiguration() {
    boolean isPortrait = orientationComboBox.getSelectedItem() != LANDSCAPE;
    int fitType;
    if(pageFitRadioButton.isSelected()) {
      fitType = PrintProcessing.PAGE_FIT;
    } else if(hFitRadioButton.isSelected()) {
      fitType = PrintProcessing.HORIZONTAL_PAGE_FIT;
    } else if(vFitRadioButton.isSelected()) {
      fitType = PrintProcessing.VERTICAL_PAGE_FIT;
    } else {
      fitType = PrintProcessing.SCALE;
    }
    PrintProcessing.setPortrait(isPortrait);
    PrintProcessing.setFitType(fitType);
    PrintProcessing.setPageFit(pageFitField.getText());
    PrintProcessing.setHorizontalPageFit(hFitField.getText());
    PrintProcessing.setVerticalPageFit(vFitField.getText());
    PrintProcessing.setScalePercentageString(scaleField.getText());
  }

  protected Point pageCount;
  protected Image image;

  protected void adjust(ClassPane classPane) {
    if(image != null) {
      image.flush();
    }
    boolean isPortrait = orientationComboBox.getSelectedItem() != LANDSCAPE;
    int fitType;
    if(pageFitRadioButton.isSelected()) {
      fitType = PrintProcessing.PAGE_FIT;
    } else if(hFitRadioButton.isSelected()) {
      fitType = PrintProcessing.HORIZONTAL_PAGE_FIT;
    } else if(vFitRadioButton.isSelected()) {
      fitType = PrintProcessing.VERTICAL_PAGE_FIT;
    } else {
      fitType = PrintProcessing.SCALE;
    }
    float pageFit = PrintProcessing.getValidVerticalPageFit(pageFitField.getText());
    float hPageFit = PrintProcessing.getValidHorizontalPageFit(hFitField.getText());
    float vPageFit = PrintProcessing.getValidVerticalPageFit(vFitField.getText());
    float scalePercentage = PrintProcessing.getValidScalePercentage(scaleField.getText());
    pageCount = PrintProcessing.getPageCount(classPane, new PrintProcessing.PrintingAttributes(isPortrait, fitType, scalePercentage, pageFit, hPageFit, vPageFit));
    if(x < 0) {
      x = 0;
    } else if(x >= pageCount.x) {
      x = pageCount.x - 1;
    }
    if(y < 0) {
      y = 0;
    } else if(y >= pageCount.y) {
      y = pageCount.y - 1;
    }
    leftButton.setEnabled(x > 0);
    rightButton.setEnabled(x < pageCount.x - 1);
    upButton.setEnabled(y > 0);
    downButton.setEnabled(y < pageCount.y - 1);
    // Start actual printing
    Point edges = PrintProcessing.getEdges(classPane, isPortrait);
    int labelSize = label.getPreferredSize().width;
    int thumbNailWidth = labelSize * edges.x / PrintProcessing.LONG_EDGE_SIZE;
    int thumbNailHeight = labelSize * edges.y / PrintProcessing.LONG_EDGE_SIZE;
    image = new BufferedImage(thumbNailWidth, thumbNailHeight, BufferedImage.TYPE_INT_RGB);
    Graphics g = image.getGraphics();
    g.setColor(Color.WHITE);
    g.fillRect(0, 0, thumbNailWidth, thumbNailHeight);
    PrintProcessing.printPage(classPane, g, new Rectangle(0, 0, thumbNailWidth, thumbNailHeight), x, y, new PrintProcessing.PrintingAttributes(isPortrait, fitType, scalePercentage, pageFit, hPageFit, vPageFit), Image.SCALE_SMOOTH);
    g.dispose();
    label.setIcon(new ImageIcon(image));
    labelPanel.setPreferredSize(new Dimension(image.getWidth(null), image.getHeight(null)));
    statusLabel.setText("Page " + (pageCount.x * y + x + 1) + "/" + (pageCount.x * pageCount.y) + " - Position " + (x + 1) + ", " + (y + 1) + "");
  }

}
