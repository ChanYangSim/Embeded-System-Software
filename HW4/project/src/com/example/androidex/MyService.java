package com.example.androidex;
//import com.example.androidex.R;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;

import android.os.Handler;


import android.widget.Toast;
//import java.util.Timer;
//import java.util.TimerTask;
import android.content.Context;

public class MyService extends Service {
	
	private static final String TAG1 = "MainActivity2";
    static int m=0;
    static int s=0;
    static int flag=0;
    int stop=0;
    Thread counter = new Thread(new Counter());
    Toast toast = null;
	 private Context ctx;
	
	 	@Override
	    public IBinder onBind(Intent arg0) 
	    {
	          return null;
	    }
	 	
	 	@Override
	    public void onCreate() 
	    {
	          super.onCreate();
	          ctx = this;	          
	          counter.start();
	    }
	 	private class Counter implements Runnable{
	 		private int count;
	 		private Handler handler = new Handler();
			@Override
			public void run() {
				// TODO Auto-generated method stub
				//final Toast toast;
				for(count=0;;count++){
					if(stop==1) break;
					m = (count%3600)/60;
					s = count%60;
					
					handler.post(new Runnable(){
						@Override
						public void run(){
								Log.d(TAG1,Integer.toString(count));
								//final Toast toast;
								toast = Toast.makeText(ctx,
										String.format("%02d:%02d",m,s),
										Toast.LENGTH_SHORT);
								toast.show();
								
								//Log.d(TAG1,Integer.toString(count));
								
								Handler shandler = new Handler();
						        shandler.postDelayed(new Runnable() {
						           @Override
						           public void run() {
						               toast.cancel(); 
						           }
						        }, 500);
								
							}
							
					});
					try{
						Thread.sleep(1000);
					}
					catch(Exception ex){
						Log.e("SampleThreadActivity", "Exception in processing message.", ex);
					}
				}
					
			}
				
		}
			 
	    @Override
	    public int onStartCommand(Intent intent, int flags, int startId){
	    	return super.onStartCommand(intent, flags, startId);
	    }
	 
	  
	    public void onDestroy() 
	    {
	          super.onDestroy();
	          if(toast!=null)
	        	  toast.cancel();
	          stop=1;
	    }
	    
	    
	    /*public MyService() {
		}*/
		/*@Override
		public void onCreate(){
			Log.d("StartService","onCreate()");
			super.onCreate();
		}
		
		@Override
		public int onStartCommand(Intent intent, int flags, int startId) {
	        Log.d("StartService","onStartCommand()");
	        String param = intent.getStringExtra("name");
	        Toast.makeText(this, "received param : "+param, Toast.LENGTH_SHORT).show();

	        return super.onStartCommand(intent, flags, startId);
		}
		
		@Override
	    public void onDestroy() {
	        Log.d("StartService","onDestroy()");
	        super.onDestroy();
	    }
		@Override
		public IBinder onBind(Intent intent) {
			// TODO: Return the communication channel to the service.
			throw new UnsupportedOperationException("Not yet implemented");
		}*/
}
