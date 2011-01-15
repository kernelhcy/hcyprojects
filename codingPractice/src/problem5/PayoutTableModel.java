package problem5;

import java.text.ParseException;
import java.text.SimpleDateFormat;

import javax.swing.table.AbstractTableModel;

public class PayoutTableModel extends AbstractTableModel
{
	
	public PayoutTableModel(IncomeAndPayoutInfo data)
	{
		this.data = data;
	}
	
	public void addPayout(MoneyItem i)
	{
		data.addPayout(i);
		fireTableDataChanged();
	}
	
	public void delPayout(int[] ins)
	{
		MoneyItem[] dels = new MoneyItem[ins.length];
		for(int i = 0; i < dels.length; ++i){
			dels[i] = data.getPayout(i);
		}
		for(int i = 0; i < dels.length; ++i){
			data.deletePayout(dels[i]);
		}
		dels = null;
		fireTableDataChanged();
	}
	
	public void delPayout(int idx)
	{
		if(idx < 0 || idx > getRowCount()){
			return;
		}
		data.deletePayout(idx);
		fireTableDataChanged();
	}
	
	@Override
	public boolean isCellEditable(int rowIndex, int columnIndex)
	{
		if(rowIndex < 0 || rowIndex > getRowCount()
			|| columnIndex < 1 || columnIndex > getColumnCount()){
			return false;
		}
		
		return true;
	}
	@Override
	public void setValueAt(Object aValue, int rowIndex, int columnIndex)
	{
		if(!isCellEditable(rowIndex, columnIndex)){
			return;
		}
		MoneyItem i = data.getPayout(rowIndex);
		switch(columnIndex){
		case 0: break;
		case 1: i.setName((String)aValue);
			break;
		case 2: i.setFromOrGoWhere((String)aValue);
			break;
		case 3: try {
				i.setTime(sdf.parse((String)aValue));
			}
			catch (ParseException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
			break;
		case 4: i.setMoney(Double.valueOf((String)aValue));
			break;
		default: 
			break;
		}
	}
	
	@Override
	public String getColumnName(int columnIndex)
	{
		return headers[columnIndex];
	}
	
	@Override
	public int getRowCount()
	{
		// TODO Auto-generated method stub
		return data.getPayoutCnt();
	}

	@Override
	public int getColumnCount()
	{
		// TODO Auto-generated method stub
		return headers.length;
	}

	@Override
	public Object getValueAt(int rowIndex, int columnIndex)
	{
		// TODO Auto-generated method stub
		if(rowIndex < 0 || rowIndex > getRowCount()
			|| columnIndex < 0 || columnIndex > getColumnCount()){
			return null;
		}
		MoneyItem i = data.getPayout(rowIndex);
		switch(columnIndex){
		case 0: return rowIndex;
		case 1: return i.getName();
		case 2: return i.getFromOrGoWhere();
		case 3: return sdf.format(i.getTime());
		case 4: return i.getMoney();
		default: return null;
		}
	}
	
	private static final String[] headers = new String[]{
		"ID", "Name", "To", "Date", "Amount"
	};
	private IncomeAndPayoutInfo data;
	private static SimpleDateFormat sdf
		= new SimpleDateFormat("yyyy/MM/dd");
}
