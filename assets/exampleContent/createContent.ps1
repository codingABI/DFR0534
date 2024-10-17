Add-Type -AssemblyName System.speech
$SpeechSynthesizer = New-Object System.Speech.Synthesis.SpeechSynthesizer
$SpeechSynthesizer.SelectVoice('Microsoft Zira Desktop')
$streamFormat = [System.Speech.AudioFormat.SpeechAudioFormatInfo]::new(16000,[System.Speech.AudioFormat.AudioBitsPerSample]::Sixteen,[System.Speech.AudioFormat.AudioChannel]::Mono)
New-Item -Name "ZH" -ItemType Directory -ErrorAction SilentlyContinue

for ($i=0; $i -le 24; $i++) {
	$SpeechSynthesizer.SetOutputToWaveFile((get-location).Path+'\ZH\'+($i).ToString('00') +'.wav',$streamFormat)
	$SpeechSynthesizer.Speak($i)
}

foreach ($i in 'A','B','C','D') {
	$SpeechSynthesizer.SetOutputToWaveFile((get-location).Path+"\ZH\0$i.wav",$streamFormat)
	$SpeechSynthesizer.Speak($i)
}


$SpeechSynthesizer.SetOutputToWaveFile((get-location).Path+"\test.wav",$streamFormat)
$SpeechSynthesizer.Speak("Test")

$SpeechSynthesizer.SetOutputToWaveFile((get-location).Path+"\hallo.wav",$streamFormat)
$SpeechSynthesizer.Speak("Hallo")

$SpeechSynthesizer.SetOutputToWaveFile((get-location).Path+"\dfr0534.wav",$streamFormat)
$SpeechSynthesizer.Speak("dfr0534")


$SpeechSynthesizer.dispose()