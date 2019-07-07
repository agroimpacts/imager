# Variables for shell examples

# setup-aws-instance.md
## Create an AMI of an existing instance
### Get instance ID of 
# instance ID of disk you want to image
```bash
IID=i-0d6b6c84c98bd9cb7
instance_name=mapper-compositer
description=Compositor_image2
aws ec2 create-image --instance-id $IID --name $instance_name --description $description
```

## Create new instance
```bash
AMIID=ami-04f0cc39f7efcb1c7
ITYPE=t2.2xlarge
KEYNAME=mapper_key_pair
SECURITY=airg-security
INAME=compositer_large
OWNER=airg
SDASIZE=70
IAM=activemapper_planet_readwriteS3

aws ec2 run-instances --image-id $AMIID --count 1 --instance-type $ITYPE --iam-instance-profile Name=$IAM --key-name $KEYNAME --security-groups $SECURITY  --block-device-mapping "[ { \"DeviceName\": \"/dev/sda1\", \"Ebs\": { \"VolumeSize\": $SDASIZE } } ]" --tag-specifications 'ResourceType=instance,Tags=[{Key=Name,Value='$INAME'}]' 'ResourceType=volume,Tags=[{Key=Owner,Value='$OWNER'}]' 
```

# Launch AWS jupyter instance
## Start the instance

In your local shell, run this:
```bash
IID=i-0d6b6c84c98bd9cb7
aws ec2 start-instances --instance-ids $IID
```


