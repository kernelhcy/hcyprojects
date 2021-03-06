package problem8;

import java.awt.FlowLayout;
import java.awt.GridLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.Calendar;
import java.util.Date;

import javax.swing.JButton;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JSpinner;
import javax.swing.JTextField;
import javax.swing.SpinnerDateModel;

import problem8.eventagent.Event;
import problem8.eventagent.EventAgent;

/**
 * Show and edit the book information.
 * @author hcy
 *
 */
public class BookInfoShower extends JPanel implements ActionListener
{
	public BookInfoShower(Book b)
	{
		if(b == null){
			throw new IllegalArgumentException(
					"BookInfoShower need a Book!");
		}
		this.b = b;
		
		ISBN = new JLabel("ISBN:");
		name = new JLabel("Name:");
		style = new JLabel("Style:");
		author = new JLabel("Author:");
		price = new JLabel("Price($):");
		ptime = new JLabel("Publish Time:");
		publisher = new JLabel("Publisher:");
		btime = new JLabel("Buy Time:");
		bower = new JLabel("Borrower:");
		
		ISBNtf = new JTextField(b.getISBN());
		nametf = new JTextField(b.getName());
		styletf = new JTextField(b.getStyle());
		authortf = new JTextField(b.getAuthor());
		pricetf = new JTextField(String.valueOf(b.getPrice()));
		
		Calendar calendar = Calendar.getInstance();
		Date initDate = calendar.getTime();
	        calendar.add(Calendar.YEAR, -100);
	        Date earliestDate = calendar.getTime();
	        calendar.add(Calendar.YEAR, 200);
	        Date latestDate = calendar.getTime();
		
		pdateModel = new SpinnerDateModel(initDate,
                                earliestDate,
                                latestDate,
                                Calendar.DAY_OF_MONTH);
		bdateModel = new SpinnerDateModel(initDate,
                                earliestDate,
                                latestDate,
                                Calendar.DAY_OF_MONTH);
		
		ptimeSpinner = new JSpinner(pdateModel);
		ptimeSpinner.setEditor(new JSpinner.DateEditor(
				ptimeSpinner, "yyyy/M/d"));
		((JSpinner.DefaultEditor)ptimeSpinner.getEditor())
			.getTextField().setText(b.getPublishTime());
		
		publishertf = new JTextField(b.getPublisher());
		
		btimeSpinner = new JSpinner(bdateModel);
		btimeSpinner.setEditor(new JSpinner.DateEditor(
				btimeSpinner, "yyyy/M/d"));
		((JSpinner.DefaultEditor)btimeSpinner.getEditor())
			.getTextField().setText(b.getBuyTime());
		bowertf = new JTextField(b.getBorrower());
		
		setLayout(new GridLayout(5, 4));
		add(ISBN);add(ISBNtf);add(style);add(styletf);
		add(name);add(nametf);add(author);add(authortf);
		add(price);add(pricetf);add(publisher);add(publishertf);
		add(ptime);add(ptimeSpinner);add(btime);add(btimeSpinner);
		add(bower);add(bowertf);
		
		save = new JButton("Save");
		save.addActionListener(this);
		JPanel p = new JPanel();
		p.setLayout(new FlowLayout(FlowLayout.CENTER, 0, 0));
		p.add(save);
		add(new JPanel());//Just occupy the grid.
		add(p);
	}
	
	/**
	 * the index of the tab pane which contains this BookInfoShower
	 * @param i
	 */
	public void setIndex(int i)
	{
		this.tabIndex = i;
	}
	
	@Override
	public void actionPerformed(ActionEvent e)
	{
		// TODO Auto-generated method stub
		b.setAuthor(authortf.getText());
		b.setBorrower(bowertf.getText());
		b.setBuyTime(((JSpinner.DefaultEditor)btimeSpinner
				.getEditor()).getTextField().getText());
		b.setISBN(ISBNtf.getText());
		b.setName(nametf.getText());
		b.setPrice(Double.valueOf(pricetf.getText()));
		b.setPublisher(publishertf.getText());
		b.setPublishTime(((JSpinner.DefaultEditor)ptimeSpinner
				.getEditor()).getTextField().getText());
		b.setStyle(styletf.getText());
		
		//update the table
		upb.clearArg();
		upb.addArg(b);
		EventAgent.dispatchEvent(upb);
		
		//update the tab name
		uptn.clearArg();
		uptn.addArg(tabIndex);
		uptn.addArg(b.getName());
		EventAgent.dispatchEvent(uptn);
	}
	
	private Book b;
	//图书的编号、书名、类型、作者、价格、出版目期、出版社、购入日期、借阅人
	private JLabel ISBN, name, style, author, price, ptime, publisher
			, btime, bower;
	private JTextField  ISBNtf, nametf, styletf, authortf, pricetf, 
			publishertf, bowertf;
	private JButton save;
	private JSpinner ptimeSpinner, btimeSpinner;
	private SpinnerDateModel pdateModel, bdateModel;
	private int tabIndex; // the index of the tab
	private Event upb = new Event("BOOKLIST_UPDATEBOOK");
	private Event uptn = new Event("JTABBEDPANE_UPDATETABNAME");
	
}
