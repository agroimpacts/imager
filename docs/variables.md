# Variables for shell examples

# setup-aws-instance.md
## Create an AMI of an existing instance
### Get instance ID of 
# instance ID of disk you want to image
```bash
IID=***REMOVED***
instance_name=mapper-compositer
description=Compositor_image2
aws ec2 create-image --instance-id $IID --name $instance_name --description $description
```

## Create new instance
```bash
AMIID=***REMOVED***
ITYPE=t2.2xlarge
KEYNAME=***REMOVED***
SECURITY=***REMOVED***
INAME=***REMOVED***
OWNER=airg
SDASIZE=70
IAM=***REMOVED***

aws ec2 run-instances --image-id $AMIID --count 1 --instance-type $ITYPE --iam-instance-profile Name=$IAM --key-name $KEYNAME --security-groups $SECURITY  --block-device-mapping "[ { \"DeviceName\": \"/dev/sda1\", \"Ebs\": { \"VolumeSize\": $SDASIZE } } ]" --tag-specifications 'ResourceType=instance,Tags=[{Key=Name,Value='$INAME'}]' 'ResourceType=volume,Tags=[{Key=Owner,Value='$OWNER'}]' 
```

# Launch AWS jupyter instance
## Start the instance

In your local shell, run this:
```bash
IID=***REMOVED***
aws ec2 start-instances --instance-ids $IID
```


