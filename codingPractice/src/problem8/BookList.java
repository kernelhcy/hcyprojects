package problem8;

import java.awt.BorderLayout;
import java.awt.Component;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.ArrayList;
import java.util.List;

import javax.swing.AbstractCellEditor;
import javax.swing.JButton;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTable;
import javax.swing.ListSelectionModel;
import javax.swing.RowFilter;
import javax.swing.RowSorter;
import javax.swing.SortOrder;
import javax.swing.event.ListSelectionEvent;
import javax.swing.event.ListSelectionListener;
import javax.swing.table.AbstractTableModel;
import javax.swing.table.TableCellEditor;
import javax.swing.table.TableRowSorter;

import problem8.eventagent.Event;
import problem8.eventagent.EventAgent;
import problem8.eventagent.EventResponser;

/**
 * Book table.
 * @author hcy
 *
 */
public class BookList extends JPanel
{
	public BookList() {
		tm = new TableModel();
		
		tb = new JTable(tm);
		tb.setColumnSelectionAllowed(false);
		tb.setRowSelectionAllowed(true);

		tb.getSelectionModel()
			.addListSelectionListener(new RowSelectionListener());
		tb.setSelectionMode(ListSelectionModel
				.MULTIPLE_INTERVAL_SELECTION);
		tb.setFillsViewportHeight(true);
		
		//sort by price and publish time
		sorter = new TableRowSorter<BookList.TableModel>(tm);
		List<RowSorter.SortKey> sortkeys 
			= new ArrayList<RowSorter.SortKey>();
		//sort by price
		sortkeys.add(new RowSorter.SortKey(4, SortOrder.ASCENDING));
		//sort by publish time
		sortkeys.add(new RowSorter.SortKey(5, SortOrder.ASCENDING));
		sorter.setSortKeys(sortkeys);
		
		tb.setRowSorter(sorter);
		
		JScrollPane jsp = new JScrollPane(tb);
		setLayout(new BorderLayout());
		add(jsp, BorderLayout.CENTER);
		
		//BOOKLIST_ROWFILTER event
		//arg[0] the regex string
		//arg[1] the column name
		EventAgent.registerEvent(new EventResponser()
		{
			
			@Override
			public String getName()
			{
				// TODO Auto-generated method stub
				return "BOOKLIST_ROWFILTER";
			}
			
			@Override
			public void doAction(Object[] args, int argc)
			{
				// TODO Auto-generated method stub
				if(argc < 2){
					return;
				}
				RowFilter<TableModel, Object> mf = null;
				String regex = (String)args[0];
				if(regex == null || regex.length() == 0){
					sorter.setRowFilter(null);
					return;
				}
				String colname = (String)args[1];
				mf = RowFilter.regexFilter(regex
					, tm.getColumnNameIndex(colname));
				sorter.setRowFilter(mf);
			}
		});
		
		//BOOKLIST_SELECTRANGE event
		//select the rows in range[arg[0], arg[1]]
		EventAgent.registerEvent(new EventResponser()
		{
			
			@Override
			public String getName()
			{
				// TODO Auto-generated method stub
				return "BOOKLIST_SELECTRANGE";
			}
			
			@Override
			public void doAction(Object[] args, int argc)
			{
				// TODO Auto-generated method stub
				if(argc < 2){
					//we need two args.
					return;
				}
				
				tb.getSelectionModel().setSelectionInterval(
						(Integer)args[0]
						,(Integer)args[1]);
			}
		});
		
		//BOOKLIST_DELSELECTED event
		//delete the selected rows
		EventAgent.registerEvent(new EventResponser()
		{
			
			@Override
			public String getName()
			{
				// TODO Auto-generated method stub
				return "BOOKLIST_DELSELECTED";
			}
			
			@Override
			public void doAction(Object[] args, int argc)
			{
				// TODO Auto-generated method stub
				if(tb.getSelectedRowCount() <= 0){
					return;
				}
				int re = JOptionPane.showConfirmDialog(me
						, "Delete ?");
				if(re == JOptionPane.CANCEL_OPTION
					|| re == JOptionPane.NO_OPTION){
					return;
				}
				
				/*
				 * During deleting the books, the index
				 * of the book will change. So we save 
				 * all the books be deleted, and then 
				 * delete them.
				 */
				ArrayList<Book> delBooks 
					= new ArrayList<Book>();
				for(int c : tb.getSelectedRows()){
					delBooks.add(tm.getBook(
						tb.convertRowIndexToModel(c)));
				}
				for(Book b : delBooks){
					tm.delBook(b);
				}
			}
		});
		
		//BOOKLIST_UPDATEBOOK event
		EventAgent.registerEvent(new EventResponser()
		{
			
			@Override
			public String getName()
			{
				// TODO Auto-generated method stub
				return "BOOKLIST_UPDATEBOOK";
			}
			
			@Override
			public void doAction(Object[] args, int argc)
			{
				// TODO Auto-generated method stub
				if(argc < 1){
					//we need the book be updated.
					return;
				}
				update((Book)args[0]);
			}
		});
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
	
	public int getBookNum()
	{
		return tb.getRowCount();
	}
	
	private TableModel tm;
	private JTable tb;
	private BookList me = this;
	private TableRowSorter<BookList.TableModel> sorter;
	
	private class RowSelectionListener implements ListSelectionListener
	{

		@Override
		public void valueChanged(ListSelectionEvent e)
		{
			if (e.getValueIsAdjusting()) {
		                return;
		        }
			
			//remove all tab
			EventAgent.dispatchEvent(rma);
			Book b;
			BookInfoShower bis;
			//show book info on the tabbedPanel
			
			for(int c: tb.getSelectedRows()){
				//We need the index of the model,
				//not the view index.
				b = tm.getBook(
					tb.convertRowIndexToModel(c));
				bis = new BookInfoShower(b);
				
				addtab.clearArg();
				if(b.getName() == null ||
					b.getName().length() == 0){
					addtab.addArg("New Book");
				}else{
					addtab.addArg(b.getName());
				}
				addtab.addArg(bis);
				EventAgent.dispatchEvent(addtab);
			}
		}
		
		private Event rma = new Event("JTABBEDPANE_REMOVEALL");
		private Event addtab = new Event("JTABBEDPANE_ADDTAB");
	}
	
	/**
	 * The editor of the date cell.
	 * @author hcy
	 *
	 */
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
		public int getColumnNameIndex(String name)
		{
			for(int i = 0; i < headerNames.length; ++i){
				if(headerNames[i].compareTo(name) == 0){
					return i;
				}
			}
			return -1;
		}
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
			if(columnIndex >= data.size()){
				/*
				 * We return the String's class.
				 * At the start time, there is no data
				 * in the table. The sortkey need the 
				 * class of the table column data.
				 * So we return the String's can prevent
				 * NullPointer exception.
				 */
				return "".getClass();
			}
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
			Object o = getValueAt(0, c);
			if(o == null){
				return null;
			}
			return o.getClass();
		}

		public boolean isCellEditable(int row, int col)
		{
			return false;
		}

		private String[] headerNames = { "ISBN", "Name", "Style",
				"Author", "Price", "Publist Time", "Publisher",
				"Buy Time", "Borrower" };
		private ArrayList<Book> data = new ArrayList<Book>();

	}
}
