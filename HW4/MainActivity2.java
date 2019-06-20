package com.example.androidex;

import com.example.androidex.R;

import android.util.Log;
import android.app.Activity;
import android.os.Bundle;
import android.content.Intent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup.Layoutparams;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.EditText;



public class MainActivity2 extends Activity{

	LinearLayout linear;
	EditText data;
	Button btn;
	OnClickListener ltn_1, ltn__2;
	BackThread mThread;

	int w = 1024;
	int blank_idx;


	public void CreatePuzzle(int row, int col){
		for(int i=0; i<row; i++){
			for(int j=0; j<col; j++){
	
			}
		}
	}


	@Override
	protected void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main2);
		button btn = (button)findViewById(R.id.button1);
		int row, col;
		ltn_1 = new OnClickListener(){
			public void onClick(View v){
				String string_data = data.getText().toString();
				row = Integer.parseInt(string_data.split(" ")[0])
				col = Integer.parseInt(string_data.split(" ")[1])
				
				CreatePuzzle(row,col);
			}
		}
		btn.setOnClickListener(ltn1);
	}

}
