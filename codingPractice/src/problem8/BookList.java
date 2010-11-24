package problem8;

import java.awt.BorderLayout;
import java.awt.Component;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.ArrayList;
import javax.swing.AbstractCellEditor;
import javax.swing.JButton;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTabbedPane;
import javax.swing.JTable;
import javax.swing.ListSelectionModel;
import javax.swing.event.ListSelectionEvent;
import javax.swing.event.ListSelectionListener;
import javax.swing.table.AbstractTableModel;
import javax.swing.table.TableCellEditor;

public class BookList extends JPanel
{
	public BookList(JTabbedPane tp) {
		this.tp = tp;
		tm = new TableModel();
		tb = new JTable(tm);
		tb.setColumnSelectionAllowed(false);
		tb.setRowSelectionAllowed(true);
		//tb.setCellSelectionEnabled(false);
		tb.getSelectionModel()
			.addListSelectionListener(new RowSelectionListener());
		tb.setSelectionMode(ListSelectionModel
				.MULTIPLE_INTERVAL_SELECTION);
		tb.setFillsViewportHeight(true);
		JScrollPane jsp = new JScrollPane(tb);
		setLayout(new BorderLayout());
		add(jsp, BorderLayout.CENTER);
	}

	public void update(Book b)
	{
		tm.update(b);
	}
	
	public void update(int row)
	{
		tm.fireTableRowsUpdated(row, row);
	}
	
	public int addBook(Book b)
	{
		return tm.addBook(b);
	}
	
	public Book delBook(Book b)
	{
		return tm.delBook(b);
	}
	
	public Book delBook(int i)
	{
		return tm.delBook(i);
	}
	
	private TableModel tm;
	private JTable tb;
	private JTabbedPane tp;
	private BookList me = this;
	
	private class RowSelectionListener implements ListSelectionListener
	{

		@Override
		public void valueChanged(ListSelectionEvent e)
		{
			if (e.getValueIsAdjusting()) {
		                return;
		        }
			
			tp.removeAll();
			Book b;
			//show book info on the tabbedPanel
			for(int c: tb.getSelectedRows()){
				b = tm.getBook(c);
				tp.addTab(b.getName()
					, new BookInfoShower(b, me));
			}
		}
		
	}
	
	private class DateEditor extends AbstractCellEditor 
				implements TableCellEditor, ActionListener
	{

		public DateEditor()
		{
			btn = new JButton();
			btn.setActionCommand(CMD);
			btn.addActionListener(this);
			btn.setBorderPainted(false);
		}
		
		@Override
		public Object getCellEditorValue()
		{
			// TODO Auto-generated method stub
			return null;
		}

		@Override
		public Component getTableCellEditorComponent(JTable table,
				Object value, boolean isSelected, int row,
				int column)
		{
			// TODO Auto-generated method stub
			return null;
		}
		
		@Override
		public void actionPerformed(ActionEvent e)
		{
			// TODO Auto-generated method stub
			
		}
		
		private JButton btn;
		private String CMD = "dateedit";
		
	}
	
	/**
	 * table model
	 * 
	 * @author hcy
	 * 
	 */
	private static class TableModel extends AbstractTableModel
	{
		public Book getBook(int i)
		{
			return data.get(i);
		}
		
		public void update(Book b)
		{
			int i = data.indexOf(b);
			System.out.println("Update: " + i + b);
			if(i >= 0){
				fireTableRowsUpdated(i, i);
			}
		}
		
		public int addBook(Book b)
		{
			if(b == null){
				return -1;
			}
			data.add(b);
			fireTableRowsInserted(data.size() -1
					, data.size() - 1);
			return data.size() - 1;
		}
		
		public Book delBook(Book b)
		{
			if(b == null){
				return b;
			}
			data.remove(b);
			fireTableDataChanged();
			return b;
		}
		
		public Book delBook(int i)
		{
			if(i < 0 && i >= data.size()){
				return null;
			}
			
			//Maybe we will search the array twice.
			Book b = data.get(i);
			delBook(b);
			return b;
		}
		
		public String getColumnName(int column) 
		{
			if(column < 0 && column >= headerNames.length){
				return null;
			}
			return headerNames[column];
		}

		public int findColumn(String columnName) 
		{
			for(int i = 0; i < headerNames.length; ++i){
				if(headerNames[i].compareTo(columnName) == 0){
					return i;
				}
			}
			return -1;
		}
		
		@Override
		public int getRowCount()
		{
			// TODO Auto-generated method stub
			return data.size();
		}

		@Override
		public int getColumnCount()
		{
			// TODO Auto-generated method stub
			return headerNames.length;
		}

		@Override
		public Object getValueAt(int rowIndex, int columnIndex)
		{
			// TODO Auto-generated method stub
			Book b = data.get(rowIndex);
			if (b == null) {
				return null;
			}
			switch (columnIndex) {
			case 0:return b.getISBN();
			case 1:return b.getName();
			case 2:return b.getStyle();
			case 3:return b.getAuthor();
			case 4:return b.getPrice();
			case 5:return b.getPublishTime();
			case 6:return b.getPublisher();
			case 7:return b.getBuyTime();
			case 8:return b.getBorrower();
			default:
			}
			return null;
		}

		public Class getColumnClass(int c)
		{
			return getValueAt(0, c).getClass();
		}

		public boolean isCellEditable(int row, int col)
		{
			return false;
		}

//		public void setValueAt(Object value, int row, int col)
//		{
//			Book b = data.get(row);
//			if (b == null) {
//				return;
//			}
//			System.out.println("Old Value: " + b);
//			switch (col) {
//			case 0:b.setISBN((String)value);break;
//			case 1:b.setName((String)value);break;
//			case 2:b.setStyle((String)value);break;
//			case 3:b.setAuthor((String)value);break;
//			case 4:b.setPrice((Double)value);break;
//			case 5:b.setPublishTime((String)value);break;
//			case 6:b.setPublisher((String)value);break;
//			case 7:b.setBuyTime((String)value);break;
//			case 8:b.setBorrower((String)value);break;
//			default:break;
//			}
//			fireTableCellUpdated(row, col);
//			System.out.println("New Value: " + b);
//		}

		private String[] headerNames = { "ISBN", "Name", "Style",
				"Author", "Price", "Publist Time", "Publisher",
				"Buy Time", "Borrower" };
		private ArrayList<Book> data = new ArrayList<Book>();

	}
}
