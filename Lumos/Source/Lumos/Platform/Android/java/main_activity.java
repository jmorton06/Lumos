package com.as.example;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;

public class main_activity extends Activity
{
	public static native int hello_cpp();

	static 
	{
		System.loadLibrary("android_studio_example");
	}

	@Override
	protected void onCreate(Bundle arg0) 
	{
		Log.d("Hello world!", "I'm Java");

		hello_cpp();

		super.onCreate(arg0);
	}
}
