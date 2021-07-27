AMID="ami id"
ITYPE="t2.xlarge"
KEYNAME="aws key name"
SECURITY="aws security group"
INAME="instance name"
OWNER="aws account number"
SDASIZE="100"
IAM="iam role"
VALIDUNTIL="2020-04-30T23:00:00"
MAXPRICE="0.08" 
aws ec2 run-instances --image-id $AMID --count 1 --instance-type $ITYPE \
--iam-instance-profile Name=$IAM --key-name $KEYNAME --security-groups $SECURITY \
--monitoring Enabled=true --block-device-mappings \
"[ { \"DeviceName\": \"/dev/sda1\", \"Ebs\": { \"VolumeSize\": $SDASIZE } } ]" \
--tag-specifications 'ResourceType=instance,Tags=[{Key=Name,Value='$INAME'}]' \
'ResourceType=volume,Tags=[{Key=Owner,Value='$OWNER'}]' \
--instance-market-options 'MarketType=spot, SpotOptions={MaxPrice='$MAXPRICE',SpotInstanceType=persistent,ValidUntil='$VALIDUNTIL', InstanceInterruptionBehavior=stop}'
