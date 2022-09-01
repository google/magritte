//
// Copyright 2020-2022 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

package com.google.privacy.magritte.demo.activities;

import android.graphics.SurfaceTexture;
import android.os.Bundle;
import androidx.appcompat.app.AppCompatActivity;
import android.util.Log;
import android.util.Size;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageButton;
import com.google.mediapipe.components.CameraHelper.CameraFacing;
import com.google.mediapipe.components.CameraXPreviewHelper;
import com.google.mediapipe.components.ExternalTextureConverter;
import com.google.mediapipe.components.FrameProcessor;
import com.google.mediapipe.components.PermissionHelper;
import com.google.mediapipe.framework.AndroidAssetUtil;
import com.google.mediapipe.glutil.EglManager;

/**
 * Main Activity class of Magritte demo app.
 *
 * <p>Note that using the mediapipe Android framework requires this to be an AppCompatActivity.
 */
public class AppMainActivity extends AppCompatActivity {
  private static final String TAG = AppMainActivity.class.getName();

  static {
    // Load mediapipe native lib.
    System.loadLibrary("mediapipe_jni");
  }

  // Sends camera-preview frames into a MediaPipe graph for processing.
  protected FrameProcessor processor;

  // Handles camera access.
  protected CameraXPreviewHelper cameraHelper;

  private ExternalTextureConverter converter;

  // {@link SurfaceTexture} where the camera-preview frames can be accessed.
  private SurfaceTexture previewSurfaceTexture;

  // ApplicationInfo for retrieving metadata defined in the manifest.
  private SurfaceView previewDisplayView;
  private EglManager eglManager;

  // Magritte face pixelation graph (optimized for front) camera
  private static final String GRAPH_NAME = "face_pixelization_live_gpu.binarypb";
  private CameraFacing cameraFacing = CameraFacing.FRONT;

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(getContentViewLayoutResId());

    ImageButton switchCamButton = findViewById(R.id.camera_toggle);
    switchCamButton.setOnClickListener(view -> flipCamera());

    previewDisplayView = new SurfaceView(this);
    setupPreviewDisplayView();

    // Initialize mediapipe asset manager to provide access to the app assets
    AndroidAssetUtil.initializeNativeAssetManager(this);

    // Create EGLManager before getting CAMERA permission
    eglManager = new EglManager(null);
    processor =
        new FrameProcessor(
            this, eglManager.getNativeContext(), GRAPH_NAME, "input_video", "output_video");
    processor.getVideoSurfaceOutput().setFlipY(true);
    Log.d(TAG, "Checking for camera permission");
    PermissionHelper.checkAndRequestCameraPermissions(this);
  }

  // Used to obtain the content view for this application. If you are extending this class, and
  // have a custom layout, override this method and return the custom layout.
  protected int getContentViewLayoutResId() {
    return R.layout.activity_main;
  }

  @Override
  protected void onResume() {
    super.onResume();
    // Make converter use the GLContext managed by eglManager.
    converter = new ExternalTextureConverter(eglManager.getContext(), 2);
    converter.setFlipY(true);
    converter.setConsumer(processor);
    if (PermissionHelper.cameraPermissionsGranted(this)) {
      startCamera();
    }
  }

  @Override
  protected void onPause() {
    super.onPause();
    converter.close();
    previewDisplayView.setVisibility(View.GONE);
  }

  @Override
  public void onRequestPermissionsResult(
      int requestCode, String[] permissions, int[] grantResults) {
    super.onRequestPermissionsResult(requestCode, permissions, grantResults);
    PermissionHelper.onRequestPermissionsResult(requestCode, permissions, grantResults);
  }

  protected void onCameraStarted(SurfaceTexture surfaceTexture) {
    previewSurfaceTexture = surfaceTexture;
    // Make the display view visible to start showing the preview. This triggers the
    // SurfaceHolder.Callback added to (the holder of) previewDisplayView.
    previewDisplayView.setVisibility(View.VISIBLE);
  }

  public void startCamera() {
    Log.d(TAG, "Starting camera");
    cameraHelper = new CameraXPreviewHelper();
    previewSurfaceTexture = converter.getSurfaceTexture();
    cameraHelper.setOnCameraStartedListener(this::onCameraStarted);
    cameraHelper.startCamera(this, cameraFacing, previewSurfaceTexture, null);
  }

  protected void onPreviewDisplaySurfaceChanged(
      SurfaceHolder holder, int format, int width, int height) {
    // (Re-)Compute the ideal size of the camera-preview display (the area that the
    // camera-preview frames get rendered onto, potentially with scaling and rotation)
    // based on the size of the SurfaceView that contains the display.
    Size viewSize = new Size(width, height);
    Size displaySize = cameraHelper.computeDisplaySizeFromViewSize(viewSize);

    // If camera has not received a frame yet, displaySize==null
    if (displaySize == null) {
      displaySize = viewSize;
    }
    boolean isCameraRotated = cameraHelper.isCameraRotated();

    // Configure the output width and height as the computed display size.
    converter.setDestinationSize(
        isCameraRotated ? displaySize.getHeight() : displaySize.getWidth(),
        isCameraRotated ? displaySize.getWidth() : displaySize.getHeight());
  }

  private void setupPreviewDisplayView() {
    previewDisplayView.setVisibility(View.GONE);
    ViewGroup viewGroup = findViewById(R.id.preview_display_layout);
    viewGroup.addView(previewDisplayView);

    previewDisplayView
        .getHolder()
        .addCallback(
            new SurfaceHolder.Callback() {
              @Override
              public void surfaceCreated(SurfaceHolder holder) {
                processor.getVideoSurfaceOutput().setSurface(holder.getSurface());
              }

              @Override
              public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
                onPreviewDisplaySurfaceChanged(holder, format, width, height);
              }

              @Override
              public void surfaceDestroyed(SurfaceHolder holder) {
                processor.getVideoSurfaceOutput().setSurface(null);
              }
            });
  }

  /** Toggle FRONT and BACK camera. */
  private void flipCamera() {
    this.cameraFacing =
        CameraFacing.FRONT.equals(cameraFacing) ? CameraFacing.BACK : CameraFacing.FRONT;
    startCamera();
  }
}
