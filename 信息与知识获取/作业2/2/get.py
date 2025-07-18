import requests
from bs4 import BeautifulSoup
from urllib.parse import urljoin
import time
import os
import json
import re

base_url = "https://www.bbc.co.uk/news/world"
headers = {
    "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36"
}

def get_article_links(page_url):
    response = requests.get(page_url, headers=headers)
    
    # 打印响应状态和部分内容以便调试
    # print(f"响应状态码: {response.status_code}")
    # print(f"页面内容预览: {response.text[:500]}...")  # 仅显示前500个字符
    
    soup = BeautifulSoup(response.text, 'html.parser')
    
    # 尝试检查页面上的所有链接标签
    all_links = soup.find_all('a')
    # print(f"页面上找到的所有链接数量: {len(all_links)}")
    
    # 使用能够找到链接的选择器
    news_links = soup.select('a[href*="/news/"]')
    # print(f"选择器'a[href*=\"/news/\"]'找到: {len(news_links)}个链接")
    
    links = []
    for a in news_links:
        href = a.get('href')
        # 放宽过滤条件，移除严格的正则表达式匹配
        if href and '/news/' in href:
            # 排除不需要的页面类型(如导航页、标签页、分类页)
            if any(x in href for x in ['/topics/', '/tags/', '/in_pictures/', '/world/']):
                continue
            # 确保链接是完整的URL
            if href.startswith('/'):
                full_url = urljoin("https://www.bbc.co.uk", href)
                links.append(full_url)
            else:
                links.append(href)
    
    # 尝试查找下一页链接
    next_page = soup.find('a', string=lambda text: text and 'Next' in text)
    next_url = urljoin(base_url, next_page['href']) if next_page else None
    
    return list(set(links)), next_url  # 使用set去除重复链接

def get_article_content(article_url):
    """获取文章的标题和内容"""
    response = requests.get(article_url, headers=headers)
    soup = BeautifulSoup(response.text, 'html.parser')
    
    # 获取文章标题 - 尝试多种可能的标题选择器
    title_elem = (
        soup.find(['h1', 'h2'], class_=lambda c: c and ('StyledHeading' in c or 'story-headline' in c)) or 
        soup.find(['h1', 'h2']) or 
        soup.find('title')
    )
    title = title_elem.text.strip() if title_elem else "无标题"
    
    # 获取文章内容 - 尝试多种可能的内容选择器
    content_blocks = (
        soup.select('article p, div[data-component="text-block"], div.ssrcss-11r1m41-RichTextComponentWrapper p') or
        soup.select('article p, [data-component="text-block"]') or
        soup.select('.story-body p, .story-body__inner p')
    )
    content = '\n\n'.join([p.text.strip() for p in content_blocks if p.text.strip()])
    
    # 检测是否为视频内容
    is_video = False
    if (
        '/av/' in article_url or 
        soup.select('[data-testid="mediaplayer"]') or 
        soup.select('video') or
        "This video can not be played" in content or
        "This video cannot be played" in content
    ):
        is_video = True
    
    return {
        'title': title,
        'url': article_url,
        'content': content,
        'is_video': is_video
    }

def save_article(article_data, output_dir='bbc_articles'):
    """保存文章到本地文件"""
    # 如果内容为空，则不保存
    if not article_data['content'].strip():
        print(f"跳过保存空内容: {article_data['title']}")
        return None
        
    # 根据内容类型确定保存路径
    if article_data.get('is_video', False):
        article_dir = os.path.join(output_dir, 'videos')
    else:
        article_dir = os.path.join(output_dir, 'articles')
    
    # 创建保存目录
    if not os.path.exists(article_dir):
        os.makedirs(article_dir)
    
    # 使用标题作为文件名，处理不适合作为文件名的字符
    filename = re.sub(r'[\\/*?:"<>|]', "_", article_data['title'])
    filename = filename.strip()[:100]  # 限制文件名长度
    
    # 保存文章内容、标题和链接
    with open(os.path.join(article_dir, f"{filename}.txt"), 'w', encoding='utf-8') as f:
        f.write(f"标题: {article_data['title']}\n\n")
        f.write(f"链接: {article_data['url']}\n\n")
        f.write(f"内容类型: {'视频' if article_data.get('is_video', False) else '文章'}\n\n")
        f.write(f"内容:\n{article_data['content']}")
    
    return os.path.join(article_dir, f"{filename}.txt")

def save_article_index(articles, output_dir='bbc_articles'):
    """保存文章索引，包含所有文章的标题和链接"""
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
    
    # 分别创建文章和视频索引
    articles_only = [a for a in articles if not a.get('is_video', False)]
    videos_only = [a for a in articles if a.get('is_video', False)]
    
    # 保存JSON索引
    with open(os.path.join(output_dir, 'index.json'), 'w', encoding='utf-8') as f:
        json.dump(articles, f, ensure_ascii=False, indent=2)
    
    # 保存文章索引
    with open(os.path.join(output_dir, 'articles_index.txt'), 'w', encoding='utf-8') as f:
        f.write(f"BBC文章索引 (共{len(articles_only)}篇)\n\n")
        for i, article in enumerate(articles_only, 1):
            f.write(f"{i}. {article['title']}\n   {article['url']}\n\n")
    
    # 保存视频索引
    with open(os.path.join(output_dir, 'videos_index.txt'), 'w', encoding='utf-8') as f:
        f.write(f"BBC视频索引 (共{len(videos_only)}篇)\n\n")
        for i, article in enumerate(videos_only, 1):
            f.write(f"{i}. {article['title']}\n   {article['url']}\n\n")

def crawl_articles(start_url, max_articles=150):  # 增加到150篇
    """爬取指定数量的文章链接"""
    all_links = []
    processed_links = set()  # 用于去重
    current_url = start_url
    
    # 如果主页面不够，可以尝试这些子分类页面
    category_urls = [
        "https://www.bbc.co.uk/news/world/asia",
        "https://www.bbc.co.uk/news/world/europe",
        "https://www.bbc.co.uk/news/world/us_and_canada",
        "https://www.bbc.co.uk/news/world/latin_america",
        "https://www.bbc.co.uk/news/world/africa",
        "https://www.bbc.co.uk/news/world/middle_east",
        "https://www.bbc.co.uk/news/world/australia"
    ]
    
    # 先爬取主页
    while current_url and len(all_links) < max_articles:
        print(f"正在爬取页面：{current_url}")
        links, next_url = get_article_links(current_url)
        
        # 添加新的未处理链接
        for link in links:
            if link not in processed_links and len(all_links) < max_articles:
                all_links.append(link)
                processed_links.add(link)
        
        current_url = next_url
        # 防止请求过快
        time.sleep(2)
    
    # 如果主页没有足够的文章，尝试子分类页面
    if len(all_links) < max_articles:
        for cat_url in category_urls:
            if len(all_links) >= max_articles:
                break
            print(f"正在爬取分类页面：{cat_url}")
            cat_links, _ = get_article_links(cat_url)
            for link in cat_links:
                if link not in processed_links and len(all_links) < max_articles:
                    all_links.append(link)
                    processed_links.add(link)
            time.sleep(1)
    
    return all_links[:max_articles]

def main():
    # 创建保存目录
    output_dir = 'bbc_articles'
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
    
    # 爬取文章链接
    print("开始爬取BBC世界新闻文章...")
    article_links = crawl_articles(base_url, max_articles=150)  # 增加到150篇
    print(f"获取到 {len(article_links)} 篇文章链接")
    
    # 获取并保存文章内容
    articles_data = []
    video_count = 0
    article_count = 0
    skipped_count = 0  # 记录跳过的空内容数量
    
    for i, link in enumerate(article_links, 1):
        try:
            print(f"正在处理第 {i}/{len(article_links)} 篇内容: {link}")
            article_data = get_article_content(link)
            
            # 跳过空内容
            if not article_data['content'].strip():
                print(f"跳过空内容页面: {article_data['title']}")
                skipped_count += 1
                continue
                
            # 分类统计视频和文章
            if article_data.get('is_video', False):
                video_count += 1
                print(f"[视频内容] {article_data['title']}")
            else:
                article_count += 1
                print(f"[文章内容] {article_data['title']}")
            
            file_path = save_article(article_data, output_dir)
            if file_path:  # 如果成功保存了文件
                print(f"已保存到: {file_path}")
                articles_data.append(article_data)
            time.sleep(1)  # 防止请求过快
        except Exception as e:
            print(f"处理内容时出错: {link}\n错误信息: {e}")
    
    # 保存索引
    save_article_index(articles_data, output_dir)
    print(f"文章索引已保存到 {output_dir}/articles_index.txt 和 {output_dir}/videos_index.txt")
    print(f"成功保存了 {article_count} 篇文章和 {video_count} 个视频到 {output_dir} 目录")
    print(f"跳过了 {skipped_count} 个空内容页面")

if __name__ == "__main__":
    main()