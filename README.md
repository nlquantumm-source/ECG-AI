# ECG AI
* Medical field is using ECG sensor in almost everyday for patient diagnosis.<br />
* By able to implement AI in simple MCU system would save more people in the world.<br />
* By simulating patient wearing shirt and measuring ECG signal, we got the noisy signal that is unable to diagnose.<br />
* By implementing this model to MCU, we achieved 66.83% signal enhancement on ECG sensors (202 BPM/s to 67 BPM/s).
* This model is based on the research paper: TinyML-Based Classification in an ECG Monitoring Embeded System by Eunchan Kim, Jaehyuk Kim, Juyoung Park, Haneul Ko, and Yeunwoong Kyung.<br /><br />


https://github.com/user-attachments/assets/69abb622-f6ed-4cf1-8dc0-5676a487aecf

  
## Software Use-case
* Use *Google Colab* when training AI model and generate header file for simple integration (Compatible for majority of the embedding engineers out there).<br />
* Once you run the Google Colab in your computer, the weighted model (for autoencoding purposes) is going to be automatically downloaded in your computer.<br />
* Use MPLAB for controlling the PIC24 (MCU) and implement autoencoding dataset from AI model to make data denoise filteration. <br />
* Since we were using PIC24 MCU which has minimum operation, we couldn't implement pre-built AI model in this device. Instead, we generate what we need.
* Our primary focus was to enhance the signal, so we used the AI technique called Autoencoder, regenerate the singal with denoise filtering.<br /><br />

## Shallow Denoising Autoencoder
* *Autoencoder Architecture*: For Encoding, input takes 32 samples and compresses it down to 16 features (Minimization of model and high accuracy). For Decoding, 16 feasures back to 32 sample using linear activation function.<br />
* *Training for Denoise*: Used Artifical Data to make the 'idealistic' ECG signal. By calculating the Mean Squared Error (MSE) between ouput and ideal clean signal, network learns to act as a noise filter, mapping noisy inputs to smooth P-QRS-T waves.<br />
* *Inference & Anomaly Detection*: If arrhythmia occurs, the Autoencoder won;t recognize the shape and will fail to reconstruct it accurately.<br /><br />

## Algorithm & Mathematical Equations<br />
| Algorithm | Mathematical Equation | Purpose/Context in Script |
| :--- | :--- | :--- |
| **Gaussian Mixture (Synthetic ECG)** | $y(t) = \sum_{i \in \{P,Q,R,S,T\}} a_i \exp\left(-\frac{(t - b_i)^2}{2c_i^2}\right)$ | Simulates a synthetic heartbeat (P-QRS-T waves) by combining Gaussian curves; Used to generate clean training data. |
| **Min-Max Normalization** | $x' = \frac{x - \min(x)}{\max(x) - \min(x)}$ | Scales the noisy ECG signals to strictly fall within a [0.0, 1.0] range; Critical for keeping neural network inputs stable. |
| **Dense (Fully Connected) Layer** | $y_j = \sum_{i} x_i \cdot W_{i,j} + B_j$ | Acts as the Encoder (compressing 32 inputs to 16 hidden features) and Decoder (expanding 16 features back to 32 outputs). |
| **ReLU Activation** | $f(x) = \max(0, x)$ | Introduces non-linearity to the hidden layer (Encoder), allowing the model to learn complex signal patterns and ignore negative values. |
| **Mean Squared Error (Loss/Error)** | $E = \sum_{i=1}^{N} (x_i - \hat{x}_i)^2$ | Used as the `loss='mse'` optimizer during Keras training, and calculated during C inference to quantify how well the AI reconstructed the signal. |
| **Threshold-based Anomaly Detection** | **Anomaly** =<br>⎨ True (0.9), if *E* > *τ*<br>⎩ False (0.1), otherwise | Acts as the logic bridge in the C script; if the reconstruction error exceeds the 0.05 threshold ($\tau$), the shape is unrecognized and flagged as an arrhythmia. |
