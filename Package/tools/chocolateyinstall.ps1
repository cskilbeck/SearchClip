[System.Reflection.Assembly]::LoadWithPartialName("System.Windows.Forms")
$oReturn=[System.Windows.Forms.MessageBox]::Show("Message Text","Title",[System.Windows.Forms.MessageBoxButtons]::OKCancel)  
switch ($oReturn){
  "OK" {
    write-host "You pressed OK"
    # Enter some code
  } 
  "Cancel" {
    write-host "You pressed Cancel"
    # Enter some code
  } 
}
