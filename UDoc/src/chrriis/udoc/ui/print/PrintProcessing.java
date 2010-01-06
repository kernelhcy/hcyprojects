/*
 * Christopher Deckers (chrriis@nextencia.net)
 * http://www.nextencia.net
 * 
 * See the file "readme.txt" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
package chrriis.udoc.ui.print;

import java.awt.Graphics;
import java.awt.Image;
import java.awt.Point;
import java.awt.Rectangle;
import java.awt.image.BufferedImage;
import java.awt.print.PageFormat;
import java.awt.print.Pageable;
import java.awt.print.Paper;
import java.awt.print.Printable;
import java.awt.print.PrinterException;
import java.awt.print.PrinterJob;
import java.util.Locale;

import javax.print.attribute.HashPrintRequestAttributeSet;
import javax.print.attribute.PrintRequestAttribute;
import javax.print.attribute.standard.JobName;
import javax.print.attribute.standard.MediaPrintableArea;
import javax.print.attribute.standard.OrientationRequested;

import chrriis.udoc.ui.ClassPane;

public class PrintProcessing {

  public static class PrintingAttributes {
    boolean isPortrait;
    int fitType;
    float scalePercentage;
    float pageFit;
    float horizontalPageFit;
    float verticalPageFit;
    public PrintingAttributes(boolean isPortrait, int fitType, float scalePercentage, float pageFit, float horizontalPageFit, float verticalPageFit) {
      this.isPortrait = isPortrait;
      this.fitType = fitType;
      this.scalePercentage = scalePercentage;
      this.pageFit = pageFit;
      this.horizontalPageFit = horizontalPageFit;
      this.verticalPageFit = verticalPageFit;
    }
  }

  public static final int SCALE = 1;
  public static final int PAGE_FIT = 4;
  public static final int HORIZONTAL_PAGE_FIT = 2;
  public static final int VERTICAL_PAGE_FIT = 3;
  protected static final int SHORT_EDGE_SIZE = 595;
  protected static final int LONG_EDGE_SIZE = 842;

  protected static String scalePercentage = "100";
  protected static String horizontalPageFit = "1";
  protected static String verticalPageFit = "1";
  protected static String pageFit = "1";
  protected static boolean isPortrait = true;

  protected static int fitType = SCALE;

  public static int getFitType() {
    return fitType;
  }

  public static void setFitType(int fitType) {
    PrintProcessing.fitType = fitType;
  }

  public static String getScalePercentageString() {
    return scalePercentage;
  }

  public static float getScalePercentage() {
    return getValidScalePercentage(scalePercentage);
  }

  public static void setScalePercentageString(String scalePercentage) {
    PrintProcessing.scalePercentage = scalePercentage;
  }

  public static String getHorizontalPageFitString() {
    return horizontalPageFit;
  }

  public static float getHorizontalPageFit() {
    return getValidHorizontalPageFit(horizontalPageFit);
  }
  
  public static void setHorizontalPageFit(String horizontalPageFit) {
    PrintProcessing.horizontalPageFit = horizontalPageFit;
  }
  
  public static String getVerticalPageFitString() {
    return verticalPageFit;
  }
  
  public static float getVerticalPageFit() {
    return getValidVerticalPageFit(verticalPageFit);
  }
  
  public static void setVerticalPageFit(String verticalPageFit) {
    PrintProcessing.verticalPageFit = verticalPageFit;
  }
  
  public static String getPageFitString() {
    return pageFit;
  }
  
  public static float getPageFit() {
    return getValidPageFit(pageFit);
  }
  
  public static void setPageFit(String pageFit) {
    PrintProcessing.pageFit = pageFit;
  }
  
  public static void setPortrait(boolean isPortrait) {
    PrintProcessing.isPortrait = isPortrait;
  }

  public static boolean isPortrait() {
    return isPortrait;
  }

  public static float getValidScalePercentage(String scalePercentage) {
    return getValidFloatValue(scalePercentage, 20, 100, 500);
  }

  public static float getValidHorizontalPageFit(String horizontalPageFit) {
    return getValidFloatValue(horizontalPageFit, 0.4f, 1, 10);
  }
  
  public static float getValidVerticalPageFit(String verticalPageFit) {
    return getValidFloatValue(verticalPageFit, 0.4f, 1, 10);
  }
  
  public static float getValidPageFit(String pageFit) {
    return getValidFloatValue(pageFit, 0.4f, 1, 10);
  }
  
  protected static float getValidFloatValue(String value, float min, float defaultValue, float max) {
    try {
      float f = Float.parseFloat(value);
      if(f > max) {
        return max;
      }
      if(f < min) {
        return min;
      }
      return f;
    } catch(Exception e) {
    }
    return defaultValue;
  }

  public static Point getEdges(ClassPane classPane, boolean isPortrait) {
    int wEdge;
    int hEdge;
    if(isPortrait) {
      wEdge = SHORT_EDGE_SIZE;
      hEdge = LONG_EDGE_SIZE;
    } else {
      wEdge = LONG_EDGE_SIZE;
      hEdge = SHORT_EDGE_SIZE;
    }
    return new Point(wEdge, hEdge);
  }
  
  protected static float computeRatio(Point edges, Rectangle printBounds, PrintingAttributes printingAttributes) {
    int w = printBounds.width;
    int h = printBounds.height;
    switch(printingAttributes.fitType) {
      case PrintProcessing.PAGE_FIT:
        return Math.min(printingAttributes.pageFit * edges.x / w, printingAttributes.pageFit * edges.y / h);
      case PrintProcessing.HORIZONTAL_PAGE_FIT:
        return printingAttributes.horizontalPageFit * edges.x / w;
      case PrintProcessing.VERTICAL_PAGE_FIT:
        return printingAttributes.verticalPageFit * edges.y / h;
      default:
        return printingAttributes.scalePercentage / 100;
    }
  }
  
  public static Point getPageCount(ClassPane classPane, PrintingAttributes printingAttributes) {
    Point edges = getEdges(classPane, printingAttributes.isPortrait);
    Rectangle printBounds = classPane.getClassComponentPanePrintBounds();
    float ratio = computeRatio(edges, printBounds, printingAttributes);
    int w = printBounds.width;
    int h = printBounds.height;
    float scaledW = w * ratio;
    float scaledH = h * ratio;
    return new Point(Math.max(0, (Math.round(scaledW) - 1)) / edges.x + 1, Math.max(0, (Math.round(scaledH) - 1)) / edges.y + 1);
  }
  
  public static float getRatio(ClassPane classPane, PrintingAttributes printingAttributes) {
    Point edges = getEdges(classPane, printingAttributes.isPortrait);
    Rectangle printBounds = classPane.getClassComponentPanePrintBounds();
    return computeRatio(edges, printBounds, printingAttributes);
  }
  
  protected static Rectangle getClassPaneImageArea(ClassPane classPane, int x, int y, PrintingAttributes printingAttributes) {
    Point edges = getEdges(classPane, printingAttributes.isPortrait);
    float ratio = getRatio(classPane, printingAttributes);
    edges.x = Math.round(edges.x / ratio);
    edges.y = Math.round(edges.y / ratio);
    int pageWidth = edges.x;
    int pageHeight = edges.y;
    return new Rectangle(x * edges.x, y * edges.y, pageWidth, pageHeight);
  }

  protected static final int MAX_WIDTH = 2000;
  protected static final int MAX_HEIGHT = 2000;
  
  public static void printPage(ClassPane classPane, Graphics g, Rectangle outputBounds, int x, int y, PrintingAttributes printingAttributes, int scalingMethod) {
    Rectangle classPaneImageArea = PrintProcessing.getClassPaneImageArea(classPane, x, y, printingAttributes);
    Rectangle printBounds = classPane.getClassComponentPanePrintBounds();
    if(printBounds == null) {
      return;
    }
    int imageWidth = classPaneImageArea.width;
    int imageHeight = classPaneImageArea.height;
    float ratio;
    int expectedHeight = Math.round((float)outputBounds.width * imageHeight / imageWidth);
    x = outputBounds.x;
    y = outputBounds.y;
    if(expectedHeight <= outputBounds.height) {
      ratio = (float)imageWidth / outputBounds.width;
    } else {
      ratio = (float)imageWidth / Math.round((float)outputBounds.height * imageWidth / imageHeight);
    }
    int divisionX = ((classPaneImageArea.width - 1) / MAX_WIDTH) + 1;
    int divisionY = ((classPaneImageArea.height - 1) / MAX_HEIGHT) + 1;
    int offsetY = 0;
    // tx and ty center the content
    int tx = Math.round((classPaneImageArea.width - ((printBounds.width - 1) % classPaneImageArea.width) + 1) / 2);
    int ty = Math.round((classPaneImageArea.height - ((printBounds.height - 1) % classPaneImageArea.height) + 1) / 2);
    for(int j=0; j<divisionY; j++) {
      int tileHeight = j == divisionY - 1? classPaneImageArea.height % MAX_HEIGHT: MAX_HEIGHT;
      int offsetX = 0;
      for(int i=0; i<divisionX; i++) {
        int tileWidth = i == divisionX - 1? classPaneImageArea.width % MAX_WIDTH: MAX_WIDTH;
        Image image = new BufferedImage(tileWidth, tileHeight, BufferedImage.TYPE_INT_ARGB);
        Graphics ig = image.getGraphics();
        ig.setClip(0, 0, tileWidth, tileHeight);
        ig.translate(tx - printBounds.x - classPaneImageArea.x - MAX_WIDTH * i, ty - printBounds.y - classPaneImageArea.y - MAX_HEIGHT * j);
        classPane.getClassComponentPane().print(ig);
        ig.dispose();
        if(scalingMethod > 0) {
          g.drawImage(image.getScaledInstance(Math.round(tileWidth / ratio), Math.round(tileHeight / ratio), BufferedImage.SCALE_SMOOTH), x + offsetX, y + offsetY, null);
        } else {
          g.drawImage(image, x + offsetX, y + offsetY, Math.round(tileWidth / ratio), Math.round(tileHeight / ratio), null);
        }
        image.flush();
        offsetX += Math.round(tileWidth / ratio);
      }
      offsetY += Math.round(tileHeight / ratio);
    }
  }

  public static void print(final ClassPane classPane) {
    PrinterJob printerJob = PrinterJob.getPrinterJob();
    PageFormat documentPageFormat = new PageFormat();
    documentPageFormat.setOrientation(PageFormat.LANDSCAPE);
    final HashPrintRequestAttributeSet hashPrintRequestAttributeSet = new HashPrintRequestAttributeSet(new PrintRequestAttribute[] {
        isPortrait? OrientationRequested.PORTRAIT: OrientationRequested.LANDSCAPE,
        new MediaPrintableArea(0, 0, Integer.MAX_VALUE, Integer.MAX_VALUE, MediaPrintableArea.INCH),
        new JobName("UDoc printing", Locale.ENGLISH),
    });
    Pageable pageable = new Pageable() {
      Point pageCount;
      public int getNumberOfPages() {
        if(pageCount == null) {
          boolean isPortrait = isPortrait();
          int fitType = getFitType();
          float scalePercentage = getScalePercentage();
          float horizontalPageFit = getHorizontalPageFit();
          float verticalPageFit = getVerticalPageFit();
          float pageFit = getPageFit();
          pageCount = getPageCount(classPane, new PrintingAttributes(isPortrait, fitType, scalePercentage, pageFit, horizontalPageFit, verticalPageFit));
        }
        return pageCount.x * pageCount.y;
      }
      public PageFormat getPageFormat(int pageIndex) throws IndexOutOfBoundsException {
        Paper paper = new Paper();
        double margin = 36; // half inch
        paper.setImageableArea(margin, margin, paper.getWidth() - margin * 2, paper.getHeight() - margin * 2);
        PageFormat pageFormat = new PageFormat();
        pageFormat.setPaper(paper);
        pageFormat.setOrientation(isPortrait()? PageFormat.PORTRAIT: PageFormat.LANDSCAPE);
        return pageFormat;
      }
      public Printable getPrintable(int pageIndex) throws IndexOutOfBoundsException {
        return new Printable() {
          public int print(Graphics graphics, PageFormat pageFormat, int pageIndex) throws PrinterException {
            int x = pageIndex % pageCount.x;
            int y = pageIndex / pageCount.x;
//            float[] area = ((MediaPrintableArea)hashPrintRequestAttributeSet.get(MediaPrintableArea.class)).getPrintableArea(MediaPrintableArea.INCH);
//            printPage(classPane, graphics, new Rectangle(Math.round(area[0] * 72), Math.round(area[1] * 72), Math.round(area[2] * 72), Math.round(area[3] * 72)), x, y, isPortrait(), getFitType(), getScalePercentage(), getHorizontalPageFit(), getVerticalPageFit(), 0);
            printPage(classPane, graphics, new Rectangle((int)Math.round(pageFormat.getImageableX()), (int)Math.round(pageFormat.getImageableY()), (int)Math.round(pageFormat.getImageableWidth()), (int)Math.round(pageFormat.getImageableHeight())), x, y, new PrintingAttributes(isPortrait(), getFitType(), getScalePercentage(), getPageFit(), getHorizontalPageFit(), getVerticalPageFit()), 0);
            return PAGE_EXISTS;
          }
        };
      }
    };
    printerJob.setPageable(pageable);
    if(printerJob.printDialog(hashPrintRequestAttributeSet)) {
      setPortrait(((OrientationRequested)hashPrintRequestAttributeSet.get(OrientationRequested.class)) == OrientationRequested.PORTRAIT);
      try {
        printerJob.print(hashPrintRequestAttributeSet);
      } catch (Exception PrintException) {
        PrintException.printStackTrace();
      }
    }
  }

}
