package com.example.androidex;

import com.example.androidex.R;

import android.graphics.Color;
import android.util.Log;
//import android.app.AlertDialog;
//import android.content.DialogInterface;
import android.app.Activity;
import android.content.Intent;
//import android.content.Context;
//import android.app.Service;
import android.os.Bundle;

import java.util.Random;

import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup.LayoutParams;

import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.EditText;


import android.util.DisplayMetrics;

public class MainActivity2 extends Activity{

	private static final String TAG = "MainActivity2";
	
	LinearLayout linear;
	EditText data;
	Button btn;
	OnClickListener ltn_1, ltn__2;
	DisplayMetrics dm;
	MainActivity2 mainActivity;
	//private Intent mIntent;
	
	//BackThread mThread;
	int W, H; // whole display
	
	int blank_idx;
	int row, col;
	int terminate=0;
	int[] dx ={-1,0,0,1};
	int[] dy ={0,-1,1,0};
	
	public boolean isCompletePuzzle(){
	
		if(blank_idx == row*col-1){
			for(int i=0;i<row*col;i++){
				Button btn = (Button)findViewById(i);

				if( Integer.valueOf(btn.getText().toString()) != i+1 ) {
					Log.d(TAG,"NOT COMPLETE!");
					return false;
				}
		
			}
			Log.d(TAG,"COMPLETE!");
			return true;
		}
		else{ // blank
			return false;
		}
		
	}
	public void EndPuzzle(){
		Log.d(TAG,"new intent");
		finish();
		Intent intent=new Intent(MainActivity2.this, MainActivity.class);
		startActivity(intent);
		

	}
	public void ShufflePuzzle(){
		Random random = new Random();
		for(int i=0;i<100; i++){
			int which = random.nextInt(4);

			if(which==0){//left
				SwapPuzzle(blank_idx-1);
			}
			else if(which==1){//up
				SwapPuzzle(blank_idx-col);
			}
			else if(which==2){//right
				SwapPuzzle(blank_idx+1);
			}
			else if(which==3){//down
				SwapPuzzle(blank_idx+col);
			}
			
		}
	}
	public void SwapPuzzle(int id){
		if(id<0 || id>=row*col) return ;
		Button blank_block = (Button)findViewById(blank_idx);
		Button move_block = (Button)findViewById(id);
		int[] blank_index = new int[2];
		int[] move_idx = new int[2];
		boolean move_flag=false;
		System.out.print(move_flag);
		blank_index[0]=blank_idx/col;
		blank_index[1]=blank_idx%col;
		move_idx[0] =id/col;
		move_idx[1] =id%col;

		for(int i=0;i<4;i++){
			if( move_idx[0]+dx[i] == blank_index[0] 
				&& move_idx[1]+dy[i] == blank_index[1]){
				move_flag=true;
				break;
			}
		}

		if(move_flag==true){
			blank_idx = id;
			//String text = blank_block.getText().toString();
			String temp = blank_block.getText().toString();
			
			blank_block.setText(move_block.getText());
			blank_block.setBackgroundColor(Color.LTGRAY);
			
			move_block.setText(temp);
			move_block.setBackgroundColor(Color.BLACK);
		}

		
	}
	public void CreatePuzzle(int row, int col){
		
			blank_idx = (row * col)-1;
			
			for(int i = 0; i < row; i++){
				LinearLayout rows = new LinearLayout(this);
				rows.setLayoutParams(new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));
				
				for(int j = 0; j < col; j++){
					int num_idx = (i*col)+ j;
					
	                // make button dynamically
					Button button_grid = new Button(this);
					button_grid.setLayoutParams(new LayoutParams(W/col, (H-150)/row));

					button_grid.setId(num_idx);
					button_grid.setBackgroundColor(Color.LTGRAY);
					
	              
					String num = String.valueOf(num_idx+1);
					button_grid.setText(num);
					
					if(num_idx == blank_idx) 
						button_grid.setBackgroundColor(Color.BLACK);
					else{
						ltn__2 = new OnClickListener(){
							public void onClick(View v){
	                        	// check button can swap and whether game finished
								SwapPuzzle(v.getId());
								if(isCompletePuzzle()){
									EndPuzzle();
									stopService(new Intent(MainActivity2.this, MyService.class));
								}
								
							}
								
						};
						
					}
					
					button_grid.setOnClickListener(ltn__2);
					rows.addView(button_grid);
					
				}
				linear.addView(rows);
			}
			
			while(isCompletePuzzle()) ShufflePuzzle();

	}
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main2);
		linear = (LinearLayout)findViewById(R.id.container);
		dm = getApplicationContext().getResources().getDisplayMetrics();
		W = dm.widthPixels;
		H = dm.heightPixels;
		
		Button btn = (Button)findViewById(R.id.button1);
		data = (EditText)findViewById(R.id.editText1);
		
		ltn_1 = new OnClickListener(){
			public void onClick(View v){
				String str_data = data.getText().toString();
				String[] str_tok = str_data.split(" ");
				
				if(str_tok.length > 2) return ;
				if(str_tok[0].equals('0') || str_tok[1].equals('0'))
					return ;
				
				row = Integer.parseInt(str_tok[0]);
				col = Integer.parseInt(str_tok[1]);
				
				CreatePuzzle(row,col);
				startService(new Intent(MainActivity2.this, MyService.class));
		
				
			}
		};
		btn.setOnClickListener(ltn_1);
	
	}

}
