$ErrorActionPreference = 'Stop'
$voice = New-Object -ComObject SAPI.SpVoice
$token = New-Object -ComObject SAPI.SpObjectToken
$stream = New-Object -ComObject SAPI.SpFileStream
try {
    $token.SetId('HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Speech\Voices\Tokens\TTS_MS_KO-KR_HEAMI_11.0')
    $voice.Voice = $token
    $voice.Rate = -2
    $voice.Volume = 85
    $stream.Format.Type = 22
    $stream.Open('C:\Users\minky\Unreal Projects\EX_1\Content\LastBroadcast\SourceAssets\IntroVoice.wav',3,$false)
    $voice.AudioOutputStream = $stream
    $voice.Speak('방송부 학생에게 알립니다. 아직 퇴실하지 않은 학생은 방송실로 오세요.') | Out-Null
    $stream.Close()
    Write-Output 'Korean announcement synthesized locally with Windows Heami.'
} finally {
    [System.Runtime.InteropServices.Marshal]::ReleaseComObject($stream) | Out-Null
    [System.Runtime.InteropServices.Marshal]::ReleaseComObject($token) | Out-Null
    [System.Runtime.InteropServices.Marshal]::ReleaseComObject($voice) | Out-Null
}
