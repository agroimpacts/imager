# Variables for shell examples

# setup-aws-instance.md
## Create an AMI of an existing instance
### Get instance ID of 
# instance ID of disk you want to image
```bash
IID=instance-id-here
instance_name=name-of-ami
description=description-of-ami
aws ec2 create-image --instance-id $IID --name $instance_name --description $description
```

## Create new instance
```bash
AMIID=ami-id
ITYPE=t2.2xlarge
KEYNAME=pem-name
SECURITY=security-group-name
INAME=new-instance-name
OWNER=owner-name
SDASIZE=volume-size-mb
IAM=iam-role-name

aws ec2 run-instances --image-id $AMIID --count 1 --instance-type $ITYPE --iam-instance-profile Name=$IAM --key-name $KEYNAME --security-groups $SECURITY  --block-device-mapping "[ { \"DeviceName\": \"/dev/sda1\", \"Ebs\": { \"VolumeSize\": $SDASIZE } } ]" --tag-specifications 'ResourceType=instance,Tags=[{Key=Name,Value='$INAME'}]' 'ResourceType=volume,Tags=[{Key=Owner,Value='$OWNER'}]' 
```

# Launch AWS jupyter instance
## Start the instance

In your local shell, run this:
```bash
IID=new-instance-id
aws ec2 start-instances --instance-ids $IID
```


