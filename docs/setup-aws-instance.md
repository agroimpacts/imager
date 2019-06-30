# Creating AWS instance

Creating an instance with Anaconda installed that can run a jupyter notebook server, if needed. This walks through the process of creating an image from an existing pre-configured instance (documentation for that is under airgresources), and then launching a new instance from that image, including expanding disk size. 

## Create an AMI of an existing instance
### Get instance ID of 
```bash
# instance ID of disk you want to image
IID=<instance_id>
instance_name=<instance_name>
description=<description>
aws ec2 create-image --instance-id $IID --name $instance_name --description $description
```

## Create new image

From the AMI you just made, create a new instance, but:

- Use a more capacious instance type (t2.2xlarge)
- Resize the volume to make it larger. Solution for increasing volume size from [here](http://blog.xi-group.com/2014/06/small-tip-use-aws-cli-to-create-instances-with-bigger-root-partitions/). 
- Attach an IAM role to allow S3 access. The code for this is from [here](https://docs.aws.amazon.com/AWSEC2/latest/UserGuide/iam-roles-for-amazon-ec2.html#launch-instance-with-role). The following set of commands will set 

The following `bash` commands will get you there:
```bash
AMIID=<ami-id>
ITYPE=<instance type>
KEYNAME=<key_pair>
SECURITY=<security_group>
INAME=<new instance name>
OWNER=<owner>
SDASIZE=<volume size in gb>
IAM=<iam role name>

aws ec2 run-instances --image-id $AMIID --count 1 --instance-type $ITYPE --iam-instance-profile Name=$IAM --key-name $KEYNAME --security-groups $SECURITY  --block-device-mapping "[ { \"DeviceName\": \"/dev/sda1\", \"Ebs\": { \"VolumeSize\": $SDASIZE } } ]" --tag-specifications 'ResourceType=instance,Tags=[{Key=Name,Value='$INAME'}]' 'ResourceType=volume,Tags=[{Key=Owner,Value='$OWNER'}]' 

```
